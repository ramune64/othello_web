const hyper_links = Array.from(document.getElementsByClassName("same_domain"));

hyper_links.forEach(element => {
    const path = element.getAttribute("href");
    fetch(path).then(response => response.text())
    .then(htmlString => {
        const parser = new DOMParser();
        const doc = parser.parseFromString(htmlString, 'text/html');
        const theme_txt = doc.getElementById("theme")?.innerText;
        const intro_txt = doc.getElementById("intro")?.innerText;
        let thumb_nail_src = doc.getElementById("thumb_nail")?.src;
        if(thumb_nail_src===undefined){
            const ogImage = doc.querySelector('meta[property="og:image"]');
            thumb_nail_src = ogImage?.getAttribute("content");
            thumb_nail_src = "/" + thumb_nail_src
        }
        
        const h5 = element.querySelector("h5");
        h5.innerText = theme_txt;
        const p = element.querySelector("p");
        p.innerText = intro_txt;
        const img = element.querySelector("img");
        img.src = thumb_nail_src;
    })
});
