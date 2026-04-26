#pragma once
template <typename... Ts> struct type_list {};

template <typename... Algs> std::string csv_header(type_list<Algs...>) {
  return ((std::string(Algs::header) + ",") + ...);
}

template <typename... Algs, typename Graph, typename params>
std::string csv_row(type_list<Algs...>, const Graph &g, const params &p) {
  return ((Algs::run(g, p) + ",") + ...);
}

template <template <typename> typename... Ts> struct type_list_tt {};

template <typename Graph, typename TypeList> struct make_tuple_from_typelist;

template <typename Graph, template <typename> typename... Algs>
struct make_tuple_from_typelist<Graph, type_list_tt<Algs...>> {
  using type = std::tuple<Algs<Graph>...>; // OK now, Algs are templates
  template <typename... Args> static auto make(Args &&...args) {
    // Forward the same argument pack to every algorithm constructor
    return std::tuple<Algs<Graph>...>(
        Algs<Graph>(std::forward<Args>(args)...)...);
  }
};

using emptyTypeList = type_list_tt<>;