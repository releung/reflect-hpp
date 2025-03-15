#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <utility>
#include <nlohmann/json.hpp>
#include "reflect.hpp"

namespace reflect_json {

inline std::string jsonToStr(nlohmann::json root) {
    return root.dump();
}

inline nlohmann::json strToJson(std::string const &json) {
    return nlohmann::json::parse(json);
}

template <class T>
struct special_traits {
    static constexpr bool value = false;
};

template <class T, std::enable_if_t<!reflect::has_member<T>() && !special_traits<T>::value, int> = 0>
nlohmann::json objToJson(T const &object) {
    return object;
}

template <class T, std::enable_if_t<!reflect::has_member<T>() && special_traits<T>::value, int> = 0>
nlohmann::json objToJson(T const &object) {
    return special_traits<T>::objToJson(object);
}

template <class T, std::enable_if_t<reflect::has_member<T>(), int> = 0>
nlohmann::json objToJson(T const &object) {
    nlohmann::json root;
    reflect::foreach_member(object, [&](const char *key, auto &value) {
        root[key] = objToJson(value);
    });
    return root;
}

template <class T, std::enable_if_t<!reflect::has_member<T>() && !special_traits<T>::value, int> = 0>
T jsonToObj(nlohmann::json const &root) {
    return root.get<T>();
}

template <class T, std::enable_if_t<!reflect::has_member<T>() && special_traits<T>::value, int> = 0>
T jsonToObj(nlohmann::json const &root) {
    return special_traits<T>::jsonToObj(root);
}

template <class T, std::enable_if_t<reflect::has_member<T>(), int> = 0>
T jsonToObj(nlohmann::json const &root) {
    T object;
    reflect::foreach_member(object, [&](const char *key, auto &value) {
        using MemberType = std::decay_t<decltype(value)>;
        if (root.contains(key)) {
            value = jsonToObj<MemberType>(root[key]);
        }
    });
    return object;
}

template <class T, class Alloc>
struct special_traits<std::vector<T, Alloc>> {
    static constexpr bool value = true;

    static nlohmann::json objToJson(std::vector<T, Alloc> const &object) {
        nlohmann::json root = nlohmann::json::array();
        for (auto const &elem: object) {
            root.push_back(reflect_json::objToJson(elem));
        }
        return root;
    }

    static std::vector<T, Alloc> jsonToObj(nlohmann::json const &root) {
        std::vector<T, Alloc> object;
        for (auto const &elem: root) {
            object.push_back(reflect_json::jsonToObj<T>(elem));
        }
        return object;
    }
};

template <class K, class V, class Comp, class Alloc>
struct special_traits<std::map<K, V, Comp, Alloc>> {
    static constexpr bool value = true;

    static nlohmann::json objToJson(std::map<K, V, Comp, Alloc> const &object) {
        nlohmann::json root;
        for (auto const &elem: object) {
            root[elem.first] = reflect_json::objToJson(elem.second);
        }
        return root;
    }

    static std::map<K, V, Comp, Alloc> jsonToObj(nlohmann::json const &root) {
        std::map<K, V, Comp, Alloc> object;
        for (auto const &[key, value]: root.items()) {
            object[key] = reflect_json::jsonToObj<V>(value);
        }
        return object;
    }
};

template <class T>
std::string serialize(T const &object) {
    return jsonToStr(objToJson(object));
}

template <class T>
T deserialize(std::string const &json) {
    return jsonToObj<T>(strToJson(json));
}

}
