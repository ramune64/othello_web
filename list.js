const article_order = ["../index.html","../how_use/how_use.html","../selfintroduce.html","../articles/article1.html","../articles/article2.html"]
const main_content = Array.from(document.getElementsByClassName("main_content"))[0];

article_order.reverse().forEach(path => {
    console.log(path)
    main_content.innerHTML += `<a href="${path}" target="_blank" class="same_domain"><div><img src="" alt="リンク先のページのサムネイル画像"><section><h5></h5><p></p></section></div></a>`;
});