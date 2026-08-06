#include <dynamic_graph/dynamic_connectivity.hpp>
#include <dycon/helpers/graph_utils.hpp>
#include <dycon/helpers/parse_command_line.hpp>
#include <parlay/internal/get_time.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

using vertex = size_t;
using utils = graph_utils<vertex>;
using edge = utils::edge;

using query = utils::query;
using queries = utils::queries;
using Pair = std::pair<size_t, size_t>;
using Ans = parlay::sequence<parlay::sequence<bool>>;
using namespace std;

int main(int argc, char** argv) {

  string usage = "Usage: Connectivity [-b #batches] [-q #queries/batch] <input_file> <output_file>";
  commandLine P(argc, argv, usage);
  auto IOF = P.IOFileNames();
  string In = IOF.first;
  string Out = IOF.second;
  size_t num_batches = P.getOptionIntValue("-b", 10);
  size_t num_queries = P.getOptionIntValue("-q", 1000);

  auto G = utils::break_sym_graph_from_bin(In);
  vertex n = G.size();

  auto E = parlay::random_shuffle(parlay::remove_duplicates(utils::to_edges(G)));
  vertex m = E.size();

  auto batch_size = parlay::tabulate(num_batches + 1, [&](size_t i) { return m / num_batches * i; });
  batch_size[num_batches] = m;

  auto batches_ins = parlay::tabulate(
      num_batches, [&](size_t i) { return parlay::to_sequence(E.cut(batch_size[i], batch_size[i + 1])); });
  auto queries_ins = utils::generate_CC_queries(num_batches, batches_ins, n, num_queries);
  Ans Ans_ins(num_batches);
  parlay::parallel_for(0, num_batches, [&](size_t i) { Ans_ins[i].resize(batches_ins[i].size()); });

  auto E_ = parlay::random_shuffle(E);
  auto batches_del = parlay::tabulate(
      num_batches, [&](size_t i) { return parlay::to_sequence(E_.cut(batch_size[i], batch_size[i + 1])); });
  auto queries_del = utils::generate_CC_queries(num_batches, batches_del, n, num_queries);
  Ans Ans_del(num_batches);
  parlay::parallel_for(0, num_batches, [&](size_t i) { Ans_del[i].resize(batches_del[i].size()); });

  parlay::internal::timer t;

  t.start();
  DynamicConnectivity graph(n);
  t.next("initialization");
  for (size_t i = 0; i < num_batches; i++) {
    for (size_t j = 0; j < batches_ins[i].size(); j++) {
      long u = batches_ins[i][j].first;
      long v = batches_ins[i][j].second;
      graph.AddEdge(UndirectedEdge(u, v));
    }
    t.next("Insert batch #" + std::to_string(i));
    for (size_t j = 0; j < queries_ins[i].size(); j++)
      Ans_ins[i][j] = graph.IsConnected(queries_ins[i][j].first, queries_ins[i][j].second);
    t.next("Answer queries #" + std::to_string(i));
  }

  for (size_t i = 0; i < num_batches; i++) {
    for (size_t j = 0; j < batches_del[i].size(); j++) {
      long u = batches_del[i][j].first;
      long v = batches_del[i][j].second;
      graph.DeleteEdge(UndirectedEdge(u, v));
    }
    t.next("Delete batch #" + std::to_string(i));
    for (size_t j = 0; j < queries_del[i].size(); j++)
      Ans_del[i][j] = graph.IsConnected(queries_del[i][j].first, queries_del[i][j].second);
    t.next("Answer queries #" + std::to_string(i));
  }

  std::ofstream faq;
  faq.open(Out);
  if (!faq.is_open()) {
    cout << "cannot open output file\n";
    std::abort();
  }
  for (size_t i = 0; i < num_batches; i++)
    for (size_t j = 0; j < queries_ins[i].size(); j++)
      faq << Ans_ins[i][j];
  // for (size_t i = 0; i < num_batches; i++)
  //   for (size_t j = 0; j < queries_del[i].size(); j++)
  //     faq << Ans_del[i][j];
  faq.close();
  return 0;
}