import io


class DotWriter():
    def __init__(self, name=None):
        self.name = name if name else 'graph'
        self._nodes = {}
        self._edges = {}
        self._indent = 0

    def add_node(self, name, **kwargs):
        self._nodes[name] = kwargs

    def add_edge(self, src, dst, **kwargs):
        self._edges[(src, dst)] = kwargs

    def read_graph(self, graph):
        nodes = set()
        for src, dst in graph.edges():
            nodes.add(src)
            nodes.add(dst)
        initial_states = getattr(graph, 'initial', set())
        accept_states = getattr(graph, 'accept', set())
        for node in nodes:
            shape = 'circle'
            if node in initial_states:
                shape = 'doublecircle'
            if node in accept_states:
                shape = 'octagon'
            self.add_node(node, shape=shape)
        for src, dst in graph.edges():
            # TODO: Add guard expressions
            # TODO: Add traversal cost
            label = ''
            if hasattr(graph, 'guard'):
                label = graph.guard(src, dst).formula
            self.add_edge(src, dst, label=label)

    def _write(self, string):
        self._buffer.write(string)

    def _write_indent(self):
        self._write(self._indent * '\t')

    def _writeline(self, line):
        self._write_indent()
        self._write(line)
        self._write('\n')

    def _assign_local_names(self):
        self._local_names = {key: f'n{i}' for i, key in enumerate(self._nodes.keys())}

    def _write_nodes(self):
        for key, attributes in self._nodes.items():
            name = self._local_names[key]
            attributes = self._nodes[key]
            shape = attributes.get('shape', 'circle')
            label = attributes.get('label', key)
            self._writeline(f'{name} [shape = {shape}; label="{label}"];')

    def _write_edges(self):
        for (src_key, dst_key), attributes in self._edges.items():
            src_name = self._local_names[src_key]
            dst_name = self._local_names[dst_key]
            label = attributes.get('label', '')
            self._writeline(f'{src_name} -> {dst_name} [label="{label}"];')

    def to_string(self):
        buffer = io.StringIO()
        self.write_to(buffer)
        output = buffer.getvalue()
        buffer.close()
        return output

    def write_to(self, buffer):
        self._buffer = buffer
        try:
            self._write_indent()
            self._write('digraph ')
            self._write(self.name if self.name else 'graph')
            self._write(' {\n')
            self._indent += 1
            self._writeline('rankdir=L,R;')
            self._writeline('size="8,5";')
            self._assign_local_names()
            self._write_nodes()
            self._write_edges()
            self._indent -= 1
            self._writeline('}')
        finally:
            self._buffer = None


if __name__ == '__main__':
    writer = DotWriter()
    writer.add_node('A', label='Node A', shape='doublecircle')
    writer.add_node('B', label='Node B')
    writer.add_edge('A', 'B', label='edgy')
    dot = writer.to_string()
    print(dot)
