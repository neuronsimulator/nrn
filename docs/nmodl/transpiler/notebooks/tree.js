define('draw_tree', ['d3'], function(d3) {

    /// draw d3 tree to given div container
    function draw_ast(container, jsontree) {

        // container margin
        var margin = {
            top: 20,
            right: 20,
            bottom: 20,
            left: 20
        };

        /// diameter equal to container width (e.g. notebook cell)
        var diameter = d3.select(container).node().getBoundingClientRect().width;

        /// width / ehight for the box
        var width = diameter;
        var height = diameter;

        /// animation duration
        var duration = 200;

        /// unique ids for nodes
        var id = 0;

        /// define tree layout
        // see: https://d3-wiki.readthedocs.io/zh_CN/master/Tree-Layout/
        var tree = d3.layout.tree()
            .size([360, diameter / 2 - 80])
            .separation(function(a, b) {
                return (a.parent == b.parent ? 1 : 10) / a.depth;
            });
        var diagonal = d3.svg.diagonal.radial()
            .projection(function(d) {
                return [d.y, d.x / 180 * Math.PI];
            });

        /// create svg element inside container
        var tree_svg = d3.select(container).append("svg")
            .attr("width", width)
            .attr("height", height)
            .append("g")
            .attr("transform", "translate(" + diameter / 2 + "," + diameter / 2 + ")");

        /// start drawing tree
        jsontree.x0 = width / 2;
        jsontree.y0 = height / 2;
        update(jsontree, tree_svg, jsontree);

        /// toggle children state on click
        function click(d, destnode, jsonroot) {
            if (d.children) {
                d._children = d.children;
                d.children = null;
            } else {
                d.children = d._children;
                d._children = null;
            }
            update(d, destnode, jsonroot);
        }

        /// collapse nodes on click
        function collapse(d) {
            if (d.children) {
                d._children = d.children;
                d._children.forEach(collapse);
                d.children = null;
            }
        }

        function update(source, destnode, jsonroot) {
            /// compute the new tree layout
            var nodes = tree.nodes(jsonroot);
            var links = tree.links(nodes);

            /// normalize for fixed-depth
            nodes.forEach(function(d) {
                d.y = d.depth * 80;
            });

            /// update the nodes
            var node = destnode.selectAll("g.node")
                .data(nodes, function(d) {
                    return d.id || (d.id = ++id);
                });

            /// enter any new nodes at the parent's previous position
            var nodeEnter = node.enter().append("g")
                .attr("class", "node")
                .on("click", function(d) {
                    click(d, destnode, jsonroot);
                });

            nodeEnter.append("circle")
                .attr("r", 1e-6)
                .style("fill", function(d) {
                    return d._children ? "lightsteelblue" : "#fff";
                });

            nodeEnter.append("text")
                .attr("x", 10)
                .attr("dy", ".55em")
                .attr("text-anchor", "start")
                .text(function(d) {
                    return d.name;
                })
                .style("fill-opacity", 1e-6);

            /// transition nodes to their new position
            var nodeUpdate = node.transition()
                .duration(duration)
                .attr("transform", function(d) {
                    return "rotate(" + (d.x - 90) + ")translate(" + d.y + ")";
                })

            nodeUpdate.select("circle")
                .attr("r", 4.5)
                .style("fill", function(d) {
                    return d._children ? "lightsteelblue" : "#fff";
                });

            nodeUpdate.select("text")
                .style("fill-opacity", 1)
                .attr("transform", function(d) {
                    return d.x < 180 ? "translate(0)" : "rotate(180)translate(-" + (d.name.length + 50) + ")";
                });

            /// appropriate transform
            var nodeExit = node.exit().transition()
                .duration(duration)
                .remove();

            nodeExit.select("circle")
                .attr("r", 1e-6);

            nodeExit.select("text")
                .style("fill-opacity", 1e-6);

            /// update the links
            var link = destnode.selectAll("path.link")
                .data(links, function(d) {
                    return d.target.id;
                });

            /// enter any new links at the parent's previous position
            link.enter().insert("path", "g")
                .attr("class", "link")
                .attr("d", function(d) {
                    var o = {
                        x: source.x0,
                        y: source.y0
                    };
                    return diagonal({
                        source: o,
                        target: o
                    });
                });

            /// transition links to their new position
            link.transition()
                .duration(duration)
                .attr("d", diagonal);

            /// transition exiting nodes to the parent's new position
            link.exit().transition()
                .duration(duration)
                .attr("d", function(d) {
                    var o = {
                        x: source.x,
                        y: source.y
                    };
                    return diagonal({
                        source: o,
                        target: o
                    });
                })
                .remove();

            /// stash the old positions for transition
            nodes.forEach(function(d) {
                d.x0 = d.x;
                d.y0 = d.y;
            });
        }

    }

    return draw_ast;
});