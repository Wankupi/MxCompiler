#include <functional>

struct Graph {
	int n = 0;
	std::vector<std::vector<int>> edges;

	explicit Graph(int n = 0) : n(n), edges(n + 1) {}
	[[nodiscard]] Graph InverseGraph() const {
		Graph G{n};
		for (int a = 0; a <= n; ++a)
			for (int b: edges[a])
				G.add_edge(b, a);
		return G;
	}
	void add_edge(int x, int y) { edges[x].push_back(y); }
	std::vector<int> const &operator[](int id) const { return edges[id]; }
};

struct GraphDfn {
	Graph &G;
	std::vector<int> fa, dfn, idfn;
	std::vector<int> size;
	int cdfn = 0;

	explicit GraphDfn(Graph &g) : G(g), fa(g.n + 1), dfn(g.n + 1), idfn(g.n + 1), size(g.n + 1) {}

	void dfs(int x) {
		dfn[x] = ++cdfn;
		idfn[cdfn] = x;
		size[x] = 1;
		for (auto y: G[x])
			if (!dfn[y]) {
				fa[y] = x;
				dfs(y);
				size[x] += size[y];
			}
	}
};


struct DominateTree {
	Graph &G;
	std::vector<int> semi, idom;

	explicit DominateTree(Graph &g) : G(g), semi(g.n + 1), idom(g.n + 1) {}
	void LengauerTarjan(int StartPointId) {
		int n = G.n;
		Graph Z = G.InverseGraph();

		GraphDfn calc_dfn(G);
		calc_dfn.dfs(StartPointId);
		calc_dfn.dfn[0] = G.n + 1;
		auto const &dfn = calc_dfn.dfn, &idfn = calc_dfn.idfn;
		auto const &parent = calc_dfn.fa;

		std::vector<int> fa(n + 1), val(n + 1);// fa: union-find-set, val: semi-dominator with min-dfn
		std::vector<std::vector<int>> idomQuery(n + 1);

		auto min_dfn = [&dfn](int a, int b) { return dfn[a] < dfn[b] ? a : b; };
		auto find = [&](int x, auto &&self) -> int {
			if (fa[x] == x) return x;
			int y = fa[x];
			fa[x] = self(fa[x], self);
			if (dfn[semi[val[x]]] > dfn[semi[val[y]]])
				val[x] = val[y];
			return fa[x];
		};
		auto merge = [&](int x, int y) {
			x = find(x, find), y = find(y, find);
			if (x == y) return;
			fa[x] = y;
		};

		for (int i = 1; i <= n; ++i) fa[i] = i;
		for (int i = n; i >= 1; --i) {
			int x = idfn[i];
			for (auto y: Z[x]) {
				if (dfn[y] < dfn[x])
					semi[x] = min_dfn(semi[x], y);
				else {
					find(y, find);
					semi[x] = min_dfn(semi[x], semi[val[y]]);
				}
			}
			for (auto y: idomQuery[x]) {
				find(y, find);
				int z = val[y];
				if (dfn[semi[z]] == dfn[x])
					idom[y] = x;
				else
					idom[y] = z;
			}
			val[x] = x;
			merge(x, parent[x]);
			idomQuery[semi[x]].push_back(x);
		}
		for (int i = 2; i <= n; ++i) {
			int x = idfn[i];
			if (idom[x] != semi[x])
				idom[x] = idom[idom[x]];
		}
	}
};

struct DominanceFrontier {
	Graph G;
	DominateTree dom;
	std::vector<std::vector<int>> out;

	DominanceFrontier(Graph &g, DominateTree &t) : G(g), dom(t), out(g.n + 1) {}
	void work() {
		int n = G.n;
		Graph tree(n);
		for (int x = 1; x <= n; ++x)
			if (dom.idom[x] != 0)
				tree.add_edge(dom.idom[x], x);
		GraphDfn gdfn(tree);
		gdfn.dfs(1);
		auto &dfn = gdfn.dfn;
		auto &siz = gdfn.size;
		for (int x = 1; x <= n; ++x)
			for (auto y: G[x]) {
				int a = dom.idom[x];
				if (a == 0) continue;
				if (!(dfn[x] <= dfn[y] && dfn[y] <= dfn[x] + siz[x] - 1)) {
					//					in[y].push_back(x);
					out[x].push_back(y);
				}
			}
		for (int i = n; i >= 1; --i) {
			int x = gdfn.idfn[i];
			int y = dom.idom[x];
			out[y].insert(out[y].end(), out[x].begin(), out[x].end());
		}
	}
};