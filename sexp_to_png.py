import os, shutil, sys, tempfile, subprocess, random, string


class BadSexpr(Exception):
    pass


def node_is_leaf(node):
    return isinstance(node, (bytes, str))


def parse_sexpr(s):
    s = s[s.find("(") :]
    tree = []
    stack = []  # top of stack (index -1) points to current node in tree
    stack.append(tree)
    curtok = ""
    depth = 0
    for c in s:
        if c == "(":
            new = []
            stack[-1].append(new)
            stack.append(new)
            curtok = ""
            depth += 1
        elif c == ")":
            if curtok:
                stack[-1].append(curtok)
                curtok = ""
            stack.pop()
            curtok = ""
            depth -= 1
        elif c.isspace():
            if curtok:
                stack[-1].append(curtok)
                curtok = ""
        else:
            curtok += c
        if depth < 0:
            raise BadSexpr("Too many closing parens")
    if depth > 0:
        raise BadSexpr("Didn't close all parens, depth %d" % depth)
    root = tree[0]
    # weird
    if isinstance(root[0], list):
        root = ["ROOT"] + root
    return root


counter = 0


def graph_tuples(node, exceptions: list = None, parent_pos=None):
    # makes both NODE and EDGE tuples from the tree
    global counter
    my_id = counter
    if node_is_leaf(node):
        col = "black"
        return [("NODE", my_id, node, {"shape": "box", "fontcolor": col, "color": col})]

    tuples = []
    name = node[0]
    if name is str:
        name = name.replace("=H", "")
    # print(exceptions)
    try:
        exceptions.index(name)
    except ValueError:
        name = f"<{name}>"

    color = "black"
    tuples.append(("NODE", my_id, name, {"shape": "none", "fontcolor": color}))

    for child in node[1:]:
        counter += 1
        child_id = counter
        opts = {}
        if len(node) > 2 and isinstance(child, list) and child[0].endswith("=H"):
            opts["arrowhead"] = "none"
            opts["style"] = "bold"
        else:
            opts["arrowhead"] = "none"
        opts["color"] = "black"
        tuples.append(("EDGE", my_id, child_id, opts))
        tuples += graph_tuples(child, exceptions=exceptions, parent_pos=name)
    return tuples


def dot_from_tuples(tuples):
    # takes graph_tuples and makes them into graphviz 'dot' format
    dot = "digraph { "
    for t in tuples:
        if t[0] == "NODE":
            more = " ".join(['%s="%s"' % (k, v) for (k, v) in t[3].items()])
            dot += """%s [label="%s" %s]; """ % (t[1], t[2], more)
        elif t[0] == "EDGE":
            more = " ".join(['%s="%s"' % (k, v) for (k, v) in t[3].items()])
            dot += """ %s -> %s [%s]; """ % (t[1], t[2], more)
    dot += "}"
    return dot


def sexp2png(
    sexpr: str, Symbols: list = [], outputFile: str = "output", format: str = "png"
):
    """
    Convert a S-Expression to png
    """
    graphviz_engine = shutil.which("dot")
    outputFile = os.path.join(os.getcwd(), f"{outputFile}.{format}")
    engine = {"png": ":cairo"}

    tree = parse_sexpr(sexpr)
    tuples = graph_tuples(tree, Symbols)
    dot = dot_from_tuples(tuples)

    tempfile = "temp_{}.gv".format(
        "".join(random.sample(string.ascii_letters + string.digits, 10))
    )
    with open(tempfile, mode="w") as tmp:
        print(dot, file=tmp)

    subprocess.Popen(
        [
            f"{graphviz_engine}",
            f"-T{format}{engine[format]}",
            f"{tempfile}",
            "-o",
            f"{outputFile}",
        ]
    ).communicate()

    os.remove(tempfile)


if __name__ == "__main__":
    sexp2png(input("S-expression:"))