#pragma once

namespace nlohmann {

	template<typename T, glm::length_t L, glm::qualifier Q>
	struct adl_serializer<glm::vec<L, T, Q>>
	{
		static void to_json(json& j, const glm::vec<L, T, Q>& v)
		{
			j = json::array();
			for (glm::length_t i = 0; i < L; ++i)
				j.push_back(v[i]);
		}

		static void from_json(const json& j, glm::vec<L, T, Q>& v)
		{
			for (glm::length_t i = 0; i < L; ++i)
				v[i] = j.at(i).get<T>();
		}

		static void to_json(ordered_json& j, const glm::vec<L, T, Q>& v)
		{
			j = ordered_json::array();
			for (glm::length_t i = 0; i < L; ++i)
				j.push_back(v[i]);
		}

		static void from_json(const ordered_json& j, glm::vec<L, T, Q>& v)
		{
			for (glm::length_t i = 0; i < L; ++i)
				v[i] = j.at(i).get<T>();
		}
	};

	template<typename T, glm::length_t C, glm::length_t R, glm::qualifier Q>
	struct adl_serializer<glm::mat<C, R, T, Q>>
	{
		static void to_json(json& j, const glm::mat<C, R, T, Q>& m)
		{
			j = json::array();
			for (glm::length_t c = 0; c < C; ++c)
			{
				json col = json::array();
				for (glm::length_t r = 0; r < R; ++r)
					col.push_back(m[c][r]);
				j.push_back(col);
			}
		}

		static void from_json(const json& j, glm::mat<C, R, T, Q>& m)
		{
			for (glm::length_t c = 0; c < C; ++c)
				for (glm::length_t r = 0; r < R; ++r)
					m[c][r] = j.at(c).at(r).get<T>();
		}

		static void to_json(ordered_json& j, const glm::mat<C, R, T, Q>& m)
		{
			j = ordered_json::array();
			for (glm::length_t c = 0; c < C; ++c)
			{
				ordered_json col = ordered_json::array();
				for (glm::length_t r = 0; r < R; ++r)
					col.push_back(m[c][r]);
				j.push_back(col);
			}
		}

		static void from_json(const ordered_json& j, glm::mat<C, R, T, Q>& m)
		{
			for (glm::length_t c = 0; c < C; ++c)
				for (glm::length_t r = 0; r < R; ++r)
					m[c][r] = j.at(c).at(r).get<T>();
		}
	};

}

