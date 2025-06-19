function bitLength(n) {
    if (n === 0) return 0;
    return n.toString(2).length;
}

function get_place(board){
    let boxes = [];
    while (board!=0){
        let last = board&-board;
        board = board-last;
        //console.log(last.toString(2).length);
        
        const col = 8-(bitLength(last)%8||8);
        //console.log(bitLength(last),col);
        const row = 8-(Math.floor((bitLength(last)-1)/8)+1);
        //console.log(col,row);
        const place = col_letters[col] + (row+1);
        //console.log(place);
        boxes.push(place);
    }
    return boxes;
}

function place_stone(white_places,black_places){
    white_places.forEach(white_place=>{
        //console.log(white_place);
        const stone_ele = document.getElementById(white_place+"_stone");
        stone_ele.classList.remove("empty");
        stone_ele.classList.remove("black");
        stone_ele.classList.add("white");
    })
    black_places.forEach(black_place=>{
        //console.log(black_place);
        const stone_ele = document.getElementById(black_place+"_stone");
        stone_ele.classList.remove("empty");
        stone_ele.classList.remove("white");
        stone_ele.classList.add("black");
    })

}

function board2place(white_board,black_board){
    const white_places = get_place(white_board);
    const black_places = get_place(black_board);
    //console.log(white_places);
    //console.log(black_places);
    place_stone(white_places,black_places);
}
function countBits(n) {
    let count = 0n;
    while (n !== 0n) {
        count += n & 1n;
        n >>= 1n;
    }
    return Number(count);
}
let colors = {};
colors[1] = "white";colors[-1] = "black";
const result_parent_ele = document.getElementById("result_parent");
const result_txt = document.getElementById("result");
const black_num_txt = document.getElementById("black_num");
const white_num_txt = document.getElementById("white_num");
const winner_txt = document.getElementById("winner");
const record_txt = document.getElementById("record");

const calculating_ele = document.getElementById("calculating");
const whitch_color = document.getElementById("whitch_color");
function update_turn(white_board,black_board){
    board2place(white_board,black_board);
    let color_index;
    let first_check_color;
    let second_check_color;
    if(current_color=="white"){
        first_check_color = "black";
        color_index = -1;
    }else{
        first_check_color = "white";
        color_index = 1;
    }
    const exist_legal1 = place_legal(first_check_color,white_board,black_board);
    if(!exist_legal1){
        color_index*=-1;
        second_check_color = colors[color_index]
        const exist_legal2 = place_legal(second_check_color,white_board,black_board);
        if(!exist_legal2){
            console.log("finish");
            const white_num = countBits(current_white);
            const black_num = countBits(current_black);
            if(white_num>black_num){
                if(pl_color==1){
                    result_txt.innerText = "勝利！！";
                    winner_txt.innerText = "あなたの勝利です。";
                }else if(pl_color==-1){
                    result_txt.innerText = "敗北...";
                    winner_txt.innerText = "CPUの勝利です。";
                }else{
                    result_txt.innerText = "終局";
                    winner_txt.innerText = "後手の勝利です。";
                }
            }else if(white_num<black_num){
                if(pl_color==-1){
                    result_txt.innerText = "勝利！！";
                    winner_txt.innerText = "あなたの勝利です。";
                }else if(pl_color==1){
                    result_txt.innerText = "敗北...";
                    winner_txt.innerText = "CPUの勝利です。";
                }else{
                    result_txt.innerText = "終局";
                    winner_txt.innerText = "先手の勝利です。";
                }
            }else{
                result_txt.innerText = "引き分け";
                winner_txt.innerText = "引き分けです。";
            }
            record_txt.innerText = record;
            black_num_txt.innerText = black_num;
            white_num_txt.innerText = white_num;
            result_parent_ele.style.display = "block";
            
            //console.log(record);
            //勝敗結果を表示
            return;
        }else{
            current_color = second_check_color;
        }
    }else{
        current_color = first_check_color;
    }
    if(current_color != colors[pl_color] && pl_color!=0){
        calculating_ele.style.display = "block";
        console.log("block");
        //console.log("minimax");
        window.setTimeout(()=>{
            calculate_CPU(cpu_LV,current_color,white_board,black_board);
        },"500")
        
        //ここにレベル別の処理と、処理中の表示、手が決まったら再帰的にこの関数を呼び出す。
    }else if(pl_color==0){
        if(current_color == "white"){
            whitch_color.classList.remove("black_turn");
            whitch_color.classList.add("white_turn");
            whitch_color.innerText = "白のターン";
        }else{
            whitch_color.classList.remove("white_turn");
            whitch_color.classList.add("black_turn");
            whitch_color.innerText = "黒のターン";
        }
    }
}

function calculate_CPU(lv,current_color,white,black){
    calculating_ele.style.display = "block";
    console.log("block");
    let act_str,_;
    //console.log(current_color,white,black);
    if(lv==0){
        const legals = get_legal_square(current_color,white,black).toJs();
        const act = legals[Math.floor(Math.random()*legals.length)];
        act_str = col_letters[7-act[1]] + ((7-act[0]+1));
    }else{
        if(current_color=="white"){
            const result = minimax(white,black,Number(lv),alpha=-Infinity,beta=Infinity,maximizing_player=true).toJs();
            [_,act_str] = result;
        }else{
            const result = minimax(black,white,Number(lv),alpha=-Infinity,beta=Infinity,maximizing_player=true).toJs();
            [_,act_str] = result;
        }
    }
    //console.log(act_str);
    const place = act_str;
    record += place;
    let flips;
    const result = identify_flip_stone(current_color,current_white,current_black,place,2).toJs();
    [current_white,current_black,flips] = result;
    current_white = BigInt(current_white);
    current_black = BigInt(current_black);
    console.log("none");
    calculating_ele.style.display = "none";
    update_turn(current_white,current_black);
}


function place_legal(coloe,white,black){
    const prev_white_legals = Array.from(document.getElementsByClassName("legal_white"));
    const prev_black_legals = Array.from(document.getElementsByClassName("legal_black"));
    prev_white_legals.forEach(prev_white_legal=>{
        prev_white_legal.classList.remove("legal_white");
        prev_white_legal.classList.add("not_legal");
    })
    prev_black_legals.forEach(prev_black_legal=>{
        prev_black_legal.classList.remove("legal_black");
        prev_black_legal.classList.add("not_legal");
    })
    if(coloe=="black"){
        const black_legals = get_legal_square("black",white,black).toJs();
        if(black_legals.length==0){
            return false;
        }
        black_legals.forEach(legal=>{
            //console.log(legal)
            legal_letter = col_letters[7-legal[1]] + ((7-legal[0]+1));
            const legal_ele = document.getElementById(legal_letter+"_legal");
            legal_ele.classList.remove("not_legal");
            legal_ele.classList.remove("legal_white");
            legal_ele.classList.add("legal_black");
            const box_ele = document.getElementById(legal_letter)
            box_ele.style.cursor = "pointer";
        })
        return true;
    }else{
        const white_legals = get_legal_square("white",white,black).toJs();
        if(white_legals.length==0){
            return false;
        }
        white_legals.forEach(legal=>{
            legal_letter = col_letters[7-legal[1]] + ((7-legal[0]+1));
            const legal_ele = document.getElementById(legal_letter+"_legal");
            legal_ele.classList.remove("not_legal");
            legal_ele.classList.remove("legal_black");
            legal_ele.classList.add("legal_white");
            const box_ele = document.getElementById(legal_letter)
            box_ele.style.cursor = "pointer";
        })
        return true;
    }
}


const col_letters = ["a","b","c","d","e","f","g","h"];
const othello_board = document.getElementById("othello_board");
let current_black,current_white;
let record = "";
function start_up(mode=0) {
    current_color = "black";
    record = "";
    if(mode==0){
        for (let row = 0; row < 8; row++) {
            for (let col = 0; col <8; col++) {
                const id_name = col_letters[col] + (row+1);
                const box = document.createElement("div");
                const stone = document.createElement("div");
                const legal = document.createElement("div");
                box.style.top = `${50+80*row}px`;
                box.style.left = `${50+80*col}px`;
                box.classList.add("box");
                box.setAttribute("id",id_name);
                stone.classList.add("stone");
                stone.classList.add("empty");
                stone.setAttribute("id",id_name+"_stone");
                legal.classList.add("legal");
                legal.classList.add("not_legal");
                legal.setAttribute("id",id_name+"_legal");
                othello_board.appendChild(box);
                box.appendChild(stone);
                box.appendChild(legal);
            }
        }
    }else{
        for (let row = 0; row < 8; row++) {
            for (let col = 0; col <8; col++) {
                const id_name = col_letters[col] + (row+1);
                const stone = document.getElementById(id_name+"_stone");
                stone.classList.remove("white");
                stone.classList.remove("black");
                stone.classList.add("empty");
            }
        }
    }
    //console.log("first_black from Python:", first_black);

    let white = BigInt(first_white);
    console.log(white);
    current_white = white;
    let black = BigInt(first_black);
    console.log(black);
    current_black = black;
    board2place(current_white,current_black);
    place_legal("black",current_white,current_black);

}


let current_color = "black";
othello_board.addEventListener("click",e=>{
    const target = e.target;
    const target_legal = Array(target.getElementsByClassName("legal"))[0][0];
    /* console.log(target_legal);
    console.log(target_legal.classList.contains("not_legal"));
    console.log(target_legal.classList); */
    if(!target_legal.classList.contains("not_legal") && (colors[pl_color] == current_color || pl_color == 0)){
        const place = target.id;
        record += place;
        //console.log(place);
        //const stone_id = place+"_stone";
        let flips;
        const result = identify_flip_stone(current_color,current_white,current_black,place,2).toJs();
        [current_white,current_black,flips] = result;
        current_white = BigInt(current_white);
        current_black = BigInt(current_black);
        //console.log(current_white);
        //console.log(current_black);
        update_turn(current_white,current_black);
    }
})

const mode_button_parent = document.getElementById("mode_button_parent");
const over_wrap = document.getElementById("over_wrap");
const color_button_parent = document.getElementById("color_button_parent");
const level_txt = document.getElementById("level");
let cpu_LV;
let pl_color = 0;
mode_button_parent.addEventListener("click",e=>{
    let target = e.target;
    if(target.classList.contains("mode_button")){

        //console.log(target);
        mode_button_parent.style.display = "none";
        if(target.id == "two"){
            mode_button_parent.style.display = "none";
            over_wrap.style.display = "none";
            pl_color = 0;
            whitch_color.innerText = "黒のターン";
            whitch_color.style.display = "block";
            level_txt.innerText="2人で対戦";
        }else{
            cpu_LV = target.id.replace("lv","");
            color_button_parent.style.display="block";
            level_txt.innerText = "CPU:LV"+cpu_LV;
        }
    }
})

color_button_parent.addEventListener("click",e=>{
    let target = e.target;
    //console.log(target);
    if(target.classList.contains("color_button")){
        //
        if(target.id=="white"){
            pl_color = 1;
            color_button_parent.style.display="none";
            over_wrap.style.display = "none";
            calculating_ele.style.display = "block";
            window.setTimeout(()=>{
                
                calculate_CPU(cpu_LV,current_color,current_white,current_black);
                calculating_ele.style.display = "none";
            },"1000")
        }else if(target.id=="black"){
            pl_color = -1;
            color_button_parent.style.display="none";
            over_wrap.style.display = "none";
        }else{
            color_button_parent.style.display="none";
            mode_button_parent.style.display = "block";
        }
    }
})

const copy_clip_board = document.getElementById("copy");
const re_copy_record = document.getElementById("re_copy_record");
const reset = document.getElementById("reset");
copy_clip_board.addEventListener("click",()=>{
    if (!navigator.clipboard) {
        alert("このブラウザは対応していません");
    return;
    }
    navigator.clipboard.writeText(record).then(
    () => {
        alert('コピー成功');
    },
    () => {
        alert('コピー失敗');
    });
})
re_copy_record.addEventListener("click",()=>{
    if (!navigator.clipboard) {
        alert("このブラウザは対応していません");
    return;
    }
    navigator.clipboard.writeText(record).then(
    () => {
        alert('コピー成功');
    },
    () => {
        alert('コピー失敗');
    });
})

const close_parent_ele = document.getElementById("close_parent");
close_parent_ele.addEventListener("click",()=>{
    result_parent_ele.style.display = "none";
    re_copy_record.style.display = "block";
    reset.style.display = "block";
});

reset.addEventListener("click",()=>{
    over_wrap.style.display="block";
    mode_button_parent.style.display="block";
    re_copy_record.style.display = "none";
    reset.style.display = "none";
    level_txt.innerText = "";
    whitch_color.style.display = "none";
    start_up(1);
})

const under_board = document.getElementById("under_board");
function scaleToFit() {
    const baseWidth = 740;
    const othello_board = document.getElementById("othello_board");
    othello_board.style.transform = `scale(${1})`;
    othello_board.style.width="100%";
    const current_width = othello_board.getBoundingClientRect().width;
    console.log("current_width:", current_width);

    // 必要に応じて scale を決定
    const scale = current_width < baseWidth ? current_width / baseWidth : 1;
    console.log(scale);
    othello_board.style.width="740px";
    othello_board.style.transform = `scale(${scale})`;
    
    //どれだけ小さくなったかを計算し、以下の要素をその分だけ上にあげる
    const baseHeight = 740;
    const real_height = baseHeight*scale;
    const offset_Y = baseHeight - real_height;
    under_board.style.transform  = `translateY(-${offset_Y}px)`;

    if(scale!==1){
        const top = real_height + 30+30+30
        calculating_ele.style.top = `${top}px`;
        whitch_color.style.top = `${top}px`;
    }else{
        calculating_ele.style.top = `0`;
        whitch_color.style.top = `0`;
    }

    const pageHeight = document.documentElement.scrollHeight;
    over_wrap.style.height = `${pageHeight}px`;
}

window.addEventListener("load", scaleToFit);
window.addEventListener("resize", scaleToFit);
