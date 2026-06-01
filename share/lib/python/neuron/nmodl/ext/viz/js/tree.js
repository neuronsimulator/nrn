// main container
var astDiv = document.getElementById("ast");

// by default show 0 to 2 depth
var collapseDepth = 2;

/// redraw AST when browser window resizes
function redrawAST() {
    /// get container and compute visible size
    var astDiv = document.getElementById("ast");
    var width = astDiv.clientWidth;
    var height = astDiv.clientHeight;

    /// ast diameter is shortest of width and height
    var diameter = width < height ? width : height;

    // define layout for tree
    var tree = d3.layout.tree()
        .size([360, diameter / 2 - 80])
        .separation(function(a, b) {
            return (a.parent == b.parent ? 7 : 10) / a.depth;
        });

    /// angle for arcs
    var diagonal = d3.svg.diagonal.radial()
        .projection(function(d) {
            return [d.y, d.x / 180 * Math.PI];
        });

    /// d3 tree should start at the centre of browser
    var centerX = width / 2;
    var centerY = height / 2;

    // any previous svg container and tooltip show be removed
    d3.select("svg").remove();
    d3.select("div.tooltip").remove();

    /// select ast svg
    var astSvg = d3.select(astDiv).append("svg")
        .attr("width", width)
        .attr("height", height)
        .append("g")
        .attr("class","treearea")
        .attr("transform", "translate(" + centerX + "," + centerY + ")");

    /// add div for tooltop
    var tooltip = d3.select("body")
        .append("div")
        .attr('class', 'tooltip')
        .style("position", "absolute")
        .style("z-index", "10")
        .style("opacity", 0)
        .style('pointer-events', 'none')

	/// set position for tree
    astRoot.x0 = height / 2;
    astRoot.y0 = 0;

    /// first need to draw then collaprse to necessary depth and then redraw
    drawTree(astRoot, astSvg, astRoot);
    collapseToDepth(astRoot, collapseDepth);
    drawTree(astRoot, astSvg, astRoot);

    /// draw main ast
    function drawTree(source, destnode, astRoot) {
        /// counter for ids
        var idCounter = 0;

        /// animation duration
        var duration = 200;

        /// compute the new tree layout
        var nodes = tree.nodes(astRoot),
            links = tree.links(nodes);

        /// normalize for fixed-depth
        nodes.forEach(function(d) {
            d.y = d.depth * 100;
        });

        /// update the nodes
        var node = destnode.selectAll("g.node")
            .data(nodes, function(d) {
                return d.id || (d.id = ++idCounter);
            });

        /// enter any new nodes at the parent's previous position
        var nodeEnter = node.enter().append("g")
            .attr("class", "node");

        /// draw little circle for each node with mouse events
        nodeEnter.append("circle")
            .attr("r", 1e-6)
            .style("fill", function(d) {
                return d._children ? "lightsteelblue" : "#fff";
            }).on("mouseover", function(d) {
                mouseover(d);
            })
            .on("mousemove", function(d) {
                mousemove(d)
            })
            .on("mouseout", function() {
                mouseout();
            })
            .on("click", function(d) {
                click(d, destnode, astRoot);
            });

        /// add ast node type as title
        nodeEnter.append("text")
            .attr("x", 10)
            .attr("dy", ".95em")
            .attr("text-anchor", "start")
            .text(function(d) {
                return d.name;
            })
            .style("fill-opacity", 1e-6);


        // transformations nodes to their new position

        var nodeUpdate = node.transition()
            .duration(duration)
            .attr("transform", function(d) {
                return "rotate(" + (d.x - 90) + ")translate(" + d.y + ")";
            })

        nodeUpdate.select("circle")
            .attr("r", 6.0)
            .style("fill", function(d) {
                return d._children ? "lightsteelblue" : "#fff";
            });

        nodeUpdate.select("text")
            .style("fill-opacity", 1)
            .attr("transform", function(d) {
                return d.x < 180 ? "translate(0)" : "rotate(180)translate(-" + (d.name.length + 50) + ")";
            });


        /// transformations when nodes are made hidden

        var nodeExit = node.exit().transition()
            .duration(duration)
            .remove();

        nodeExit.select("circle")
            .attr("r", 1e-6);

        nodeExit.select("text")
            .style("fill-opacity", 1e-6);

        /// update the link
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

        /// setup zoom listener
        d3.select("svg")
            .call(d3.behavior.zoom().scaleExtent([0.2, 5]).on("zoom", zoom));
    }

    /// make tooltip visible
    function mouseover(d) {
        if (d.nmodl) {
            tooltip.text(d.nmodl);
        } else {
            tooltip.text(d.name);
        }
        d3.select('div.tooltip').
            transition()
            .delay(100)
            .duration(300)
            .style("opacity", 100)
            .style('pointer-events', 'none')
        return tooltip;
    }

    /// move tooltip with text as move moves
    function mousemove(d) {
        var height = Math.round(Number(tooltip.style('height').slice(0, -2)));
        var top = (d3.event.pageY - (height / 2.0)) + "px";
        var left = (d3.event.pageX + 15) + "px";
        return tooltip.style("top", top).style("left", left);
    }

    /// hide tooltip if mouse goes out
    function mouseout() {
        d3.select('div.tooltip').
            transition()
            .delay(100)
            .duration(300)
            .style("opacity", 0)
            .style('pointer-events', 'none');
        return tooltip;
    }

    /// toggle children on click
    function click(d, destnode, astRoot) {
        if (d.children) {
            d._children = d.children;
            d.children = null;
        } else {
            d.children = d._children;
            d._children = null;
        }
        drawTree(d, destnode, astRoot);
    }

    /// collapse nodes
    function collapse(d) {
        if (d.children) {
            d._children = d.children;
            d._children.forEach(collapse);
            d.children = null;
        }
    }

    /// collaprse ast to given depth
    function collapseToDepth(node, depth) {
        if (node.depth > depth) {
            collapse(node);
        }
        if (node.depth <= depth) {
            node.children = node.children || node._children;
        }
        if (!node.children) {
            return;
        }
        node.children.forEach(function(n) {
            collapseToDepth(n, depth);
        });
    }

    /// zoom-in/out ast drawing area
    function zoom() {
        var scale = d3.event.scale;
        d3.select(".treearea")
          .attr("transform", "translate(" + centerX + "," + centerY + ") scale(" + scale + ")");
    }
}


/// draw for the first time
redrawAST();

/// Redraw based on the new size whenever the browser window is resized
window.addEventListener("resize", redrawAST);
