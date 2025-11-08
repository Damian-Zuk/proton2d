#pragma once

#include <atomic>
#include <utility>
#include <cassert>
#include <type_traits>

namespace proton {

    struct ControlBlockBase
    {
        std::atomic<uint32_t> Strong{ 0 };
        std::atomic<uint32_t> Weak{ 0 };

        ControlBlockBase() = default;
        virtual ~ControlBlockBase() = default;

        virtual void DestroyObject() noexcept = 0;

        uint32_t StrongCount() const noexcept { return Strong.load(std::memory_order_acquire); }
        uint32_t WeakCount()   const noexcept { return Weak.load(std::memory_order_acquire); }
    };

    template<typename T>
    struct ControlBlock : ControlBlockBase
    {
        T* ptr{ nullptr };

        explicit ControlBlock(T* p) noexcept 
            : ptr(p) 
        { 
            Strong.store(1, std::memory_order_release);
            Weak.store(0, std::memory_order_release);
        }
        ~ControlBlock() override = default;

        void DestroyObject() noexcept override
        {
            delete ptr;
            ptr = nullptr;
        }
    };

    // -----------------------------
    // Intrusive support
    // -----------------------------
    // If you inherit from IntrusiveRefCounted<T>, the object will create its own control-block at construction.
    // The control block will hold a pointer to the object. When the Strong-count goes to zero the object
    // will be deleted by the control block pathway; the control block remains until Weak-count reaches zero.
    template<typename T>
    class IntrusiveRefCounted
    {
    public:
        IntrusiveRefCounted() noexcept
        {
            m_Control = new ControlBlock<T>(static_cast<T*>(this));
            // We already created the control block with Strong == 1 (meaning there is one "intrusive owner"
            // if you consider the object's own existence a "Strong reference"). To avoid confusing semantics,
            // we will set the initial Strong count to 0 and treat explicit Ref creation as increments.
            // But the easiest/safer approach is to set Strong==0 here and let callers call IncStrong when needed.
            // To keep behavior intuitive for "Ref(this)" we will initialize Strong==0:
            m_Control->Strong.store(0, std::memory_order_release);
            m_Control->Weak.store(0, std::memory_order_release);
        }

        virtual ~IntrusiveRefCounted()
        {
            // When object is destroyed, it should null out the pointer inside control block so WeakRef can see expiration.
            if (m_Control)
            {
                m_Control->ptr = nullptr;
            }
        }

        // Access to control block (used by Ref/WeakRef helpers)
        ControlBlock<T>* GetControlBlock() const noexcept { return m_Control; }

        // Note: copying the object will *not* copy the control block. If you need copy semantics,
        // handle the control block behavior in derived classes.
    protected:
        ControlBlock<T>* m_Control{ nullptr };
    };

    // -----------------------------
    // Forward declarations
    // -----------------------------
    template<typename T> class Ref;
    template<typename T> class WeakRef;
    template<typename T> class Scope;

    // -----------------------------
    // Ref<T>
    // -----------------------------
    template<typename T>
    class Ref
    {
    public:
        Ref() noexcept : m_CB(nullptr) {}
        Ref(std::nullptr_t) noexcept : m_CB(nullptr) {}

        // Construct from a control-block (internal)
        explicit Ref(ControlBlockBase* cb) noexcept : m_CB(cb)
        {
            if (m_CB)
            {
                m_CB->Strong.fetch_add(1, std::memory_order_acq_rel);
            }
        }

        // Construct from raw pointer *only* when pointer is intrusive (has control-block inside)
        // Use SFINAE to enable this ctor only if T inherits IntrusiveRefCounted<T>.
        template<typename U = T, typename = std::enable_if_t<std::is_base_of<IntrusiveRefCounted<U>, U>::value>>
        explicit Ref(U* raw) noexcept
        {
            if (raw)
            {
                ControlBlock<U>* cb = raw->GetControlBlock();
                m_CB = cb;
                // increment Strong count
                cb->Strong.fetch_add(1, std::memory_order_acq_rel);
            }
            else
            {
                m_CB = nullptr;
            }
        }

        // copy
        Ref(const Ref& other) noexcept : m_CB(other.m_CB)
        {
            if (m_CB)
                m_CB->Strong.fetch_add(1, std::memory_order_acq_rel);
        }

        // move
        Ref(Ref&& other) noexcept : m_CB(other.m_CB)
        {
            other.m_CB = nullptr;
        }

        ~Ref() { release(); }

        Ref& operator=(const Ref& other) noexcept
        {
            if (this != &other)
            {
                release();
                m_CB = other.m_CB;
                if (m_CB) m_CB->Strong.fetch_add(1, std::memory_order_acq_rel);
            }
            return *this;
        }

        Ref& operator=(Ref&& other) noexcept
        {
            if (this != &other)
            {
                release();
                m_CB = other.m_CB;
                other.m_CB = nullptr;
            }
            return *this;
        }

        // create Ref from raw T* when we know it has an intrusive cb
        template<typename U = T, typename = std::enable_if_t<std::is_base_of<IntrusiveRefCounted<U>, U>::value>>
        static Ref FromRaw(U* raw) noexcept { return Ref(raw); }

        // extract raw pointer (may be nullptr)
        T* Get() const noexcept
        {
            if (!m_CB) return nullptr;
            auto cb = static_cast<ControlBlock<T>*>(m_CB);
            return cb->ptr;
        }

        T* operator->() const noexcept { return Get(); }
        T& operator*() const noexcept { return *Get(); }

        explicit operator bool() const noexcept { return Get() != nullptr; }

        uint32_t UseCount() const noexcept
        {
            return m_CB ? m_CB->StrongCount() : 0;
        }

        bool operator==(const Ref& o) const noexcept { return Get() == o.Get(); }
        bool operator!=(const Ref& o) const noexcept { return !(*this == o); }

        // allow WeakRef to access m_CB
        template<typename> friend class WeakRef;

    private:
        void release() noexcept
        {
            if (!m_CB) return;

            // decrement Strong count
            uint32_t old = m_CB->Strong.fetch_sub(1, std::memory_order_acq_rel);
            assert(old != 0); // underflow indicates bug

            // if we were last Strong owner -> destroy object
            if (old == 1)
            {
                // destroy managed object (must be safe to call from any thread)
                m_CB->DestroyObject();

                // if no Weak refs -> delete control block
                if (m_CB->Weak.load(std::memory_order_acquire) == 0)
                {
                    delete m_CB;
                    m_CB = nullptr;
                    return;
                }
            }

            // otherwise, if there are still Weak refs, keep control block alive.
            m_CB = nullptr;
        }

        ControlBlockBase* m_CB;
    };

    // -----------------------------
    // WeakRef<T>
    // -----------------------------
    template<typename T>
    class WeakRef
    {
    public:
        WeakRef() noexcept : m_CB(nullptr) {}
        WeakRef(std::nullptr_t) noexcept : m_CB(nullptr) {}

        // from a Ref<T>
        WeakRef(const Ref<T>& ref) noexcept : m_CB(ref.m_CB)
        {
            if (m_CB)
                m_CB->Weak.fetch_add(1, std::memory_order_acq_rel);
        }

        // from another WeakRef
        WeakRef(const WeakRef& other) noexcept : m_CB(other.m_CB)
        {
            if (m_CB)
                m_CB->Weak.fetch_add(1, std::memory_order_acq_rel);
        }

        WeakRef(WeakRef&& other) noexcept : m_CB(other.m_CB)
        {
            other.m_CB = nullptr;
        }

        ~WeakRef() { release(); }

        WeakRef& operator=(const WeakRef& other) noexcept
        {
            if (this != &other)
            {
                release();
                m_CB = other.m_CB;
                if (m_CB) m_CB->Weak.fetch_add(1, std::memory_order_acq_rel);
            }
            return *this;
        }

        WeakRef& operator=(WeakRef&& other) noexcept
        {
            if (this != &other)
            {
                release();
                m_CB = other.m_CB;
                other.m_CB = nullptr;
            }
            return *this;
        }

        // create from raw intrusive pointer
        template<typename U = T, typename = std::enable_if_t<std::is_base_of<IntrusiveRefCounted<U>, U>::value>>
        explicit WeakRef(U* raw) noexcept
        {
            if (raw)
            {
                ControlBlock<U>* cb = raw->GetControlBlock();
                m_CB = cb;
                m_CB->Weak.fetch_add(1, std::memory_order_acq_rel);
            }
            else
            {
                m_CB = nullptr;
            }
        }

        // Lock -> Ref<T> (empty if expired)
        Ref<T> Lock() const noexcept
        {
            if (!m_CB) return nullptr;

            // Try to increment Strong only if Strong != 0 and pointer still valid.
            uint32_t curStrong = m_CB->Strong.load(std::memory_order_acquire);
            while (curStrong != 0)
            {
                if (m_CB->Strong.compare_exchange_weak(curStrong, curStrong + 1, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    // success
                    return Ref<T>(m_CB);
                }
                // otherwise curStrong updated; loop
            }
            // expired
            return nullptr;
        }

        bool Expired() const noexcept
        {
            if (!m_CB) return tue;
            // expired if Strong == 0 or underlying pointer is null
            if (m_CB->StrongCount() == 0) return true;
            // check pointer for null (if derived ControlBlock<T>)
            auto typed = dynamic_cast<ControlBlock<T>*>(m_CB);
            if (typed)
                return typed->ptr == nullptr;
            return false;
        }

    private:
        void release() noexcept
        {
            if (!m_CB) return;

            uint32_t old = m_CB->Weak.fetch_sub(1, std::memory_order_acq_rel);
            assert(old != 0);

            // if Weak was last and there are no Strong refs -> delete control block
            if (old == 1 && m_CB->Strong.load(std::memory_order_acquire) == 0)
            {
                delete m_CB;
            }

            m_CB = nullptr;
        }

        ControlBlockBase* m_CB;

        template<typename> friend class Ref;
    };

    // -----------------------------
    // Scope<T> (unique-like)
    // -----------------------------
    template<typename T>
    class Scope
    {
    public:
        Scope() noexcept = default;
        explicit Scope(T* ptr) noexcept : m_Ptr(ptr) {}
        ~Scope() { delete m_Ptr; }

        Scope(Scope&& other) noexcept : m_Ptr(other.m_Ptr) { other.m_Ptr = nullptr; }
        Scope& operator=(Scope&& other) noexcept { if (this != &other) { delete m_Ptr; m_Ptr = other.m_Ptr; other.m_Ptr = nullptr; } return *this; }

        // non-copyable
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

        T* Get() const noexcept { return m_Ptr; }
        T* operator->() const noexcept { return m_Ptr; }
        T& operator*() const noexcept { return *m_Ptr; }
        explicit operator bool() const noexcept { return m_Ptr != nullptr; }

    private:
        T* m_Ptr{ nullptr };
    };

    // -----------------------------
    // MakeRef / MakeScope helpers
    // -----------------------------

    // MakeRef - allocate object and control block together (non-intrusive)
    template<typename T, typename... Args>
    Ref<T> MakeRef(Args&&... args)
    {
        // create object then control block that owns it
        T* obj = new T(std::forward<Args>(args)...);
        ControlBlock<T>* cb = new ControlBlock<T>(obj);
        return Ref<T>(cb);
    }

    // MakeScope
    template<typename T, typename... Args>
    Scope<T> MakeScope(Args&&... args)
    {
        return Scope<T>(new T(std::forward<Args>(args)...));
    }

} // namespace Core
