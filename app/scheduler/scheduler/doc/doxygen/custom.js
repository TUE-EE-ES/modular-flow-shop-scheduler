$(function () {
    $(".fragment").each(function (i, node) {
        var $node = $(node);
        $node.children(":not(.line)").remove();
        $node.html("<pre><code class='stan'>" + $node.text().trim().replaceAll("\<", "&lt;").replaceAll("\>", "&gt;") + "</code></pre>");
    });
    hljs.highlightAll();
});
