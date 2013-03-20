// original version from
// http://stackoverflow.com/questions/12150491/toc-list-with-all-classes-generated-by-automodule-in-sphinx-docs
// added support for methods -- 2013-01-19
// modified to just list method name not header -- 2013-01-19
// modified to put methods etc in paragraph instead of a list -- 2013-03-20
$(function (){
var createList = function(selector){

    var ul = $('<p>');
    var selected = $(selector);

    if (selected.length === 0){
        return;
    }

    selected.clone().each(function (i,e){

        var p = $(e).children('.descclassname');
        var n = $(e).children('.descname');
        var l = $(e).children('.headerlink');

        var a = $('<a>');
        a.attr('href',l.attr('href')).attr('title', 'Link to this definition');

        //a.append(p).append(n);
        a.append(n);

        ul.append(a);
        ul.append(' &middot; ');
    });
    return ul;
}



var c = $('<div>');

var ul0 = c.clone().append($('.submodule-index'))

customIndex = $('.custom-index');
customIndex.empty();
customIndex.append(ul0);

var x = [];
x.push(['Classes','dl.class > dt']);
x.push(['Functions','dl.function > dt']);
x.push(['Variables','dl.data > dt']);
x.push(['Methods','dl.method > dt']);
x.forEach(function (e){
    var l = createList(e[1]);
    if (l) {        
        var ul = c.clone()
            .append('<p class="rubric">'+e[0]+'</p>')
            .append(l);
    }
    customIndex.append(ul);
});

});

