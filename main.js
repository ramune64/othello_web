console.log("main.js: スクリプトの実行を開始しました。");


const firebaseConfigDev  = {
    apiKey: "AIzaSyARbLoBNG0h1JDUZWgmTO5uQaFFaGjL4r0",
    authDomain: "e-coach-ai-dev.firebaseapp.com",
    databaseURL: "https://e-coach-ai-dev-default-rtdb.firebaseio.com",
    projectId: "e-coach-ai-dev",
    storageBucket: "e-coach-ai-dev.firebasestorage.app",
    messagingSenderId: "594985646322",
    appId: "1:594985646322:web:2986bf3d12b1693e85f86b"
};
const firebaseConfigProd  = {
    apiKey: "AIzaSyCQY-RCNAOKjvp8ItF4SJOr3iCgNjZSrGM",
    authDomain: "e-coach-ai.firebaseapp.com",
    databaseURL: "https://e-coach-ai-default-rtdb.firebaseio.com",
    projectId: "e-coach-ai",
    storageBucket: "e-coach-ai.firebasestorage.app",
    messagingSenderId: "250769341822",
    appId: "1:250769341822:web:5d49eff013adeef941c72f",
    measurementId: "G-VJJVMNV2HX"
};

let selectedFirebaseConfig;

// 現在のホスト名に基づいて Firebase 設定を選択
if (window.location.hostname === "localhost" || window.location.hostname === "127.0.0.1") {
    console.log("Local development environment detected. Using DEV Firebase project.");
    selectedFirebaseConfig = firebaseConfigDev;
} else if (window.location.hostname === "e-coach-ai.com" || window.location.hostname === "e-coach-ai.firebaseapp.com") {
    console.log("Production environment detected. Using PROD Firebase project.");
    selectedFirebaseConfig = firebaseConfigProd;
} else {
    console.warn("Unknown environment. Defaulting to DEV Firebase project. Please check hostname:", window.location.hostname);
  selectedFirebaseConfig = firebaseConfigDev; // またはエラーをスロー
}


// Initialize Firebase (compat バージョンは firebase グローバル変数を使用)
// firebase-app-compat.js を読み込むと window.firebase が使えるようになる
const app = firebase.initializeApp(selectedFirebaseConfig);
const analytics = firebase.analytics(); // firebase-analytics-compat.js を読み込むと使える
const database = firebase.database(); // firebase-database-compat.js を読み込むと使える
const functions = firebase.functions(); // ここで firebase.functions() を呼び出してインスタンスを取得
const firestoreClient = firebase.firestore();
/**
 * 日本時間 (JST) で今日の日付を 'YYYY-MM-DD' 形式で取得します。
 * クライアントのローカルタイムゾーンに関わらず、常にJSTで処理します。
 */
function getJSTDateString(date = new Date()) {
    // Intl.DateTimeFormat を使用して、指定したタイムゾーンで日付をフォーマットします。
    // ほとんどのモダンブラウザは 'Asia/Tokyo' タイムゾーンをサポートしています。
    const formatter = new Intl.DateTimeFormat('ja-JP', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        timeZone: 'Asia/Tokyo'
    });
    // formatToParts を使用して、日付の各部分を抽出して 'YYYY-MM-DD' 形式を構築します。
    const parts = formatter.formatToParts(date);
    const year = parts.find(p => p.type === 'year')?.value;
    const month = parts.find(p => p.type === 'month')?.value;
    const day = parts.find(p => p.type === 'day')?.value;
    return `${year}-${month}-${day}`;
}

/**
 * 日本時間 (JST) で指定された日数前の日付を 'YYYY-MM-DD' 形式で取得します。
 * 日数計算もJST基準で行います。
 */
function getJSTPastDateString(daysAgo) {
    // JSTの今日の日付文字列を取得し、それを元にDateオブジェクトを作成することで、
    // クライアントのローカルタイムゾーンの影響を受けずにJSTでの日数計算を可能にします。
    const jstTodayString = getJSTDateString(new Date());
    const [year, month, day] = jstTodayString.split('-').map(Number);
    // Date.UTC を使用して、JSTの日付をUTCとして設定します (例: 2023-11-20 00:00 UTC)
    const jstDate = new Date(Date.UTC(year, month - 1, day));

    // UTCの日付から指定日数分を減算します
    jstDate.setUTCDate(jstDate.getUTCDate() - daysAgo);

    // 減算後の日付を再びJSTとしてフォーマットします
    const formatter = new Intl.DateTimeFormat('ja-JP', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        timeZone: 'Asia/Tokyo'
    });
    const parts = formatter.formatToParts(jstDate);
    const pastYear = parts.find(p => p.type === 'year')?.value;
    const pastMonth = parts.find(p => p.type === 'month')?.value;
    const pastDay = parts.find(p => p.type === 'day')?.value;
    return `${pastYear}-${pastMonth}-${pastDay}`;
}
/**
 * 2日以上前のデータを削除します。
 * recordCpuGameResult が実行される際にトリガーされます。
 */

async function cleanupOldDailyCpuStats() {
    //const database = window.database;
    const twoDaysAgoJST = getJSTPastDateString(2); // 日本時間で2日前の日付を取得 (例: 2023-11-18)
    const dailyStatsRef = database.ref('daily_cpu_stats');

    try {
        const snapshot = await dailyStatsRef.once('value'); // daily_cpu_stats の全てのデータを一度取得
        if (snapshot.exists()) {
        const allDates = snapshot.val();
        const dateKeys = Object.keys(allDates || {}); // 存在する全ての日付キー (例: ["2023-11-17", "2023-11-18", "2023-11-19", ...])

        for (const dateKey of dateKeys) {
            // 日付文字列の辞書順比較を利用して、古い日付を判定します。
            // 例: "2023-11-17" < "2023-11-18" は true となり、古いデータと判断されます。
            if (dateKey <= twoDaysAgoJST) {
            console.log(`古いデータ (${dateKey}) を削除します...`);
            await database.ref(`daily_cpu_stats/${dateKey}`).set(null); // nullを設定することでノードを削除
            }
        }
        }
    } catch (error) {
        console.error("古いデータのクリーンアップ中にエラーが発生しました:", error);
    }
}

let currentUserUid = null; // 現在の匿名ユーザーのUIDを保持する変数

// アプリのロード時に匿名サインインを試みる関数
async function signInAnonymouslyOnce() {
    if (firebase.auth().currentUser) {
        currentUserUid = firebase.auth().currentUser.uid;
        console.log("匿名ユーザーとして既にサインイン済みです！UID:", currentUserUid);
        return;
    }
    try {
        const userCredential = await firebase.auth().signInAnonymously(); // firebase.auth() を使う
        currentUserUid = userCredential.user.uid;
        console.log("匿名ユーザーとしてサインインしました！UID:", currentUserUid);
        // ★追加: Firestore に最終ログイン日時を記録
        try {
            const userRef = firestoreClient.collection('users').doc(currentUserUid);
            console.log("got_UID:",currentUserUid);
            await userRef.set({
                lastLoginAt: firebase.firestore.FieldValue.serverTimestamp()
            }, { merge: true }); // merge: true で既存フィールドを上書きしない
            console.log(`Firestore: ユーザー ${currentUserUid} の最終ログイン日時を更新しました。`);
        } catch (firestoreError) {
            console.error("Firestore: 最終ログイン日時更新中にエラーが発生しました:", firestoreError);
        }
    } catch (error) {
        console.error("匿名サインイン中にエラーが発生しました:", error);
        // エラーが発生したら、currentUserUid は null のまま
    }
    
}

const recordOthelloResultCallable = functions.httpsCallable('recordOthelloResult');

async function save_result(cpuResult,lv,record){
    // まず、匿名認証が完了していることを確認
    console.log("save start");
    if (!firebase.auth().currentUser) { // firebase.auth().currentUser で現在のユーザーを確認
        console.warn("ユーザーがサインインしていません。匿名サインインを試みます。");
        await signInAnonymouslyOnce(); // 匿名サインインが完了するまで待つ
        if (!firebase.auth().currentUser) { // 再度確認。サインインに失敗した場合はここで終了
            console.error("匿名サインインに失敗したため、ゲーム結果を送信できません。");
            alert("ゲーム結果を記録できませんでした。もう一度お試しください。");
            return;
        }
    }
    console.log("save_result: Cloud Function を呼び出す直前。");
    console.log("save_result: firebase.auth().currentUser:", firebase.auth().currentUser); // ここが null でないこと
    if (firebase.auth().currentUser) {
        console.log("save_result: currentUser.uid:", firebase.auth().currentUser.uid); // UID が表示されること
        try {
            const idTokenResult = await firebase.auth().currentUser.getIdTokenResult();
            console.log("save_result: currentUser.getIdTokenResult().token:", idTokenResult.token); // IDトークンが表示されること
            // ★重要: これが null や undefined ではないこと。そして、取得時にエラーが出ないこと。
        } catch (tokenError) {
            console.error("save_result: IDトークン取得エラー:", tokenError); // エラーが出ないこと
        }
    } else {
        console.log("save_result: currentUser は null です。"); // ★このログが出ていたら問題
    }
    try {
        console.log("通信開始");
        // Callable Cloud Function を呼び出す
        const result = await recordOthelloResultCallable({
            cpuLevel: lv,
            cpuResult: cpuResult,
            record :record,
        });
        console.log("Cloud Functionからの応答:", result.data);

        // 必要であれば、Cloud Functionの応答に基づいてUIを更新
        if (result.data.status === 'success') {
            console.log(`ゲーム結果が正常にサーバーに記録されました。CPUレベル${lv}`);
            // ここで displayDailyCpuGameStats を呼び出して、UIを更新しても良いでしょう
            // displayDailyCpuGameStats(lv); 
        } else {
            console.warn(`ゲーム結果の記録に問題が発生しました: ${result.data.message}`);
        }

    } catch (error) {
    console.error("Cloud Functionの呼び出し中にエラーが発生しました:", error.message);
    if (error.code === 'unauthenticated') {
        console.error("認証が必要です。再度サインインを試みてください。");
    } else if (error.code === 'invalid-argument') {
        console.error("無効な引数がCloud Functionに渡されました。");
    } else if (error.code === 'resource-exhausted') { // レートリミットのエラーハンドリング
        alert("リクエストが多すぎます。1分後に再度ゲーム結果を送信できます。");
        console.warn("レートリミットに達しました。");
    } else {
        console.error("その他の不明なエラー:", error.code);
    }
}
}


async function save_result2(cpu_result,lv){
    await cleanupOldDailyCpuStats();
    //const database = window.database;
    const todayJST = getJSTDateString(); // 日本時間での今日の日付を取得
    const statsRef = database.ref(`daily_cpu_stats/${todayJST}/cpu_level_${lv}`);
    statsRef.transaction((currentData) => {
        const data = currentData || { wins: 0, losses: 0, draws: 0 };
        if (cpu_result === 1) data.wins++;
        else if (cpu_result === -1) data.losses++;
        else if (cpu_result === 0) data.draws++;
        return data;
    })
    .then((result) => {
        if (result.committed) {
            console.log(`CPUレベル${lv}との今日の勝敗が正常に更新されました！`);
        } else {
            console.log(`CPUレベル${lv}との今日の勝敗更新はコミットされませんでした (トランザクションが中断された可能性)。`);
        }
    })
    .catch((error) => {
        console.error(`CPUレベル${lv}との勝敗更新中にエラーが発生しました:`, error);
    });
}

async function getDailyCpuGameStats(cpuLevel) {
    const todayJST = getJSTDateString();
    const statsRef = database.ref(`daily_cpu_stats/${todayJST}/cpu_level_${cpuLevel}`);

    try {
        // once('value') は Promise を返すので await で結果を待てます
        const snapshot = await statsRef.once('value');
        const data = snapshot.val();

        if (data) {
            const wins = data.wins || 0;
            const losses = data.losses || 0;
            const draws = data.draws || 0;
            console.log(`JST:${todayJST} CPUレベル${cpuLevel}: 勝利 ${wins} 回, 敗北 ${losses} 回, 引き分け ${draws} 回`);
            return { wins, losses, draws };
        } else {
            console.log(`JST:${todayJST} CPUレベル${cpuLevel}: まだゲーム結果が記録されていません。`);
            return { wins: 0, losses: 0, draws: 0 };
        }
    } catch (error) {
        console.error(`CPUレベル${cpuLevel}の統計取得中にエラーが発生しました:`, error);
        return { wins: 0, losses: 0, draws: 0 }; // エラー時もデフォルト値を返す
    }
}

async function displayDailyCpuGameStats() {
    console.log("日次CPUゲーム統計の表示を開始します...");
    const allStats = {}; // 全CPUレベルの統計を格納するオブジェクト

    for (let cpuLevel = 0; cpuLevel <= 9; cpuLevel++) {
        // await を使って、各CPUレベルの統計取得が完了するのを待つ
        const stats = await getDailyCpuGameStats(cpuLevel);
        allStats[cpuLevel] = stats;
        
        // ここでUIを更新する処理を追加できます
        // 例: document.getElementById(`cpu-stats-level-${cpuLevel}`).innerText = `勝利: ${stats.wins}, 敗北: ${stats.losses}`;
        // console.log(`表示用: CPUレベル${cpuLevel} - 勝利: ${stats.wins}, 敗北: ${stats.losses}, 引き分け: ${stats.draws}`);
    }
    const stats = await getDailyCpuGameStats(85);
    allStats[85] = stats;
    console.log("すべてのCPUレベルの統計取得が完了しました:", allStats);
    for (let cpuLevel = 0; cpuLevel <= 9; cpuLevel++) {
        const id_name = `lv_${cpuLevel}win_rate`;
        const lv_parent = document.getElementById(id_name);
        lv_parent.querySelector(".num_win").textContent = allStats[cpuLevel].losses;
        lv_parent.querySelector(".num_lose").textContent = allStats[cpuLevel].wins;
        lv_parent.querySelector(".num_draw").textContent = allStats[cpuLevel].draws;
    }
    let cpuLevel = 85;
    const id_name = `lv_${cpuLevel}win_rate`;
    const lv_parent = document.getElementById(id_name);
    lv_parent.querySelector(".num_win").textContent = allStats[cpuLevel].losses;
    lv_parent.querySelector(".num_lose").textContent = allStats[cpuLevel].wins;
    lv_parent.querySelector(".num_draw").textContent = allStats[cpuLevel].draws;
    return allStats; // 必要に応じて集計結果を返す
}
displayDailyCpuGameStats();

async function fetchUserBestWins() {
    if (!currentUserUid) {
        console.warn("ユーザーがサインインしていません。勝利記録を取得できません。");
        // サインインを待つ、あるいはサインインさせる
        await signInAnonymouslyOnce();
        if (!currentUserUid) {
            console.error("サインインに失敗したため、勝利記録を取得できません。");
            return null;
        }
    }

    try {
        const doc = await firestoreClient.collection('user_best_wins').doc(currentUserUid).get();
        if (doc.exists) {
            console.log("ユーザーの勝利記録を取得しました:", doc.data());
            return doc.data(); // ドキュメントのデータを返す
        } else {
            console.log("このユーザーの勝利記録はまだありません。");
            return {}; // 空のオブジェクトを返す
        }
    } catch (error) {
        console.error("ユーザーの勝利記録の取得中にエラーが発生しました:", error);
        return null;
    }
}

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

function place_stone(white_places,black_places,last_placed,flips){
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
    for (let row = 0; row < 8; row++) {
        for (let col = 0; col <8; col++) {
            const id_name = col_letters[col] + (row+1);
            const box_ele = document.getElementById(id_name);
            box_ele.style.backgroundColor = "rgba(0,0,0,0)";
        }
    }
    if(last_placed){
        document.getElementById(last_placed).style.backgroundColor="gray";
    }
    if(flips){
        flips.forEach(element => {
            let flip_str = convert_act_bit2str(element);
            document.getElementById(flip_str).style.backgroundColor="rgba(128,128,128,0.7)"
        });
    }

}

function board2place(white_board,black_board,last_placed,flips){
    const white_places = get_place(white_board);
    const black_places = get_place(black_board);
    //console.log(white_places);
    //console.log(black_places);
    place_stone(white_places,black_places,last_placed,flips);
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
function update_turn(white_board,black_board,last_placed,flips){
    board2place(white_board,black_board,last_placed,flips);
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
            let cpu_result;
            const white_num = countBits(current_white);
            const black_num = countBits(current_black);
            if(white_num>black_num){
                if(pl_color==1){
                    result_txt.innerText = "勝利！！";
                    winner_txt.innerText = "あなたの勝利です。";
                    cpu_result = -1;
                }else if(pl_color==-1){
                    result_txt.innerText = "敗北...";
                    winner_txt.innerText = "CPUの勝利です。";
                    cpu_result = 1;
                }else{
                    result_txt.innerText = "終局";
                    winner_txt.innerText = "後手の勝利です。";
                }
            }else if(white_num<black_num){
                if(pl_color==-1){
                    result_txt.innerText = "勝利！！";
                    winner_txt.innerText = "あなたの勝利です。";
                    cpu_result = -1;
                }else if(pl_color==1){
                    result_txt.innerText = "敗北...";
                    winner_txt.innerText = "CPUの勝利です。";
                    cpu_result = 1;
                }else{
                    result_txt.innerText = "終局";
                    winner_txt.innerText = "先手の勝利です。";
                }
            }else{
                result_txt.innerText = "引き分け";
                winner_txt.innerText = "引き分けです。";
                cpu_result = 0;
            }
            record_txt.innerText = record;
            black_num_txt.innerText = black_num;
            white_num_txt.innerText = white_num;
            result_parent_ele.style.display = "block";
            

            if(pl_color!==0){
                save_result(Number(cpu_result),Number(cpu_LV),record);
            }
            
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
        //console.log("block");
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
async function callMinimax(white, black, lv, pl) {
    const resultProxy = await run_minimax(white, black, lv, pl);
    const result = resultProxy.toJs();  // ✅ Proxy を明示的にJSオブジェクトに変換

    const { row, col, score } = result;
    console.log("選ばれた手:", row, col, "評価値:", score);

    return result;
}





async function runMinimax(board_w, board_b, lv, pl) {
    /* const w_high = board_w >> BigInt(32);                  // 上位32bit
    const w_low  = board_w & BigInt(0xFFFFFFFF);           // 下位32bit

    const w_high_num = Number(w_high);
    const w_low_num  = Number(w_low);

    const b_high = board_b >> BigInt(32);                  // 上位32bit
    const b_low  = board_b & BigInt(0xFFFFFFFF);           // 下位32bit

    const b_high_num = Number(b_high);
    const b_low_num  = Number(b_low); */

    const act_ptr = Module._malloc(8);    // 2 int
    const score_ptr = Module._malloc(4);  // 1 float

    //console.log(w_high,w_low,b_high,b_low);

    try {
        const white_num = countBits(current_white);
        const black_num = countBits(current_black);
        const turn = (white_num + black_num)-4+1;
    if(pl==1){
        if(lv==8.5){
            if(turn>=42){
                console.log("turn_final:",turn);
                await minimax_old_c(BigInt(board_w),BigInt(board_b), Number(12), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }else{
                console.log("turn_nomal:",turn);
                await minimax_old_c(BigInt(board_w),BigInt(board_b), Number(8), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }
        }
        if(lv==9){
            if(turn>=42){
                console.log("turn_final:",turn);
                await minimax_c(BigInt(board_w),BigInt(board_b), Number(12), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }else{
                console.log("turn_nomal:",turn);
                await minimax_c(BigInt(board_w),BigInt(board_b), Number(8), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }
        }else{
            await minimax_c(BigInt(board_w),BigInt(board_b), Number(lv), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
        }
    }else{
        if(lv==8.5){
            if(turn>=42){
                console.log("turn_final:",turn);
                await minimax_old_c(BigInt(board_b),BigInt(board_w), Number(12), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }else{
                console.log("turn_nomal:",turn);
                await minimax_old_c(BigInt(board_b),BigInt(board_w), Number(8), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }
        }
        if(lv==9){
            if(turn>=42){
                console.log("turn_final:",turn);
                await minimax_c(BigInt(board_b),BigInt(board_w), Number(12), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }else{
                console.log("turn_nomal:",turn);
                await minimax_c(BigInt(board_b),BigInt(board_w), Number(8), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
            }
            
        }else{
            await minimax_c(BigInt(board_b),BigInt(board_w), Number(lv), Number(-9999999.0), Number(9999999.0),1, 0, 0, act_ptr, score_ptr);
        }
    }
    }catch (error) {
        console.error('Error:', error);
    }
    console.log("finish");

    const row = Module.HEAP32[act_ptr >> 2];
    const col = Module.HEAP32[(act_ptr >> 2) + 1];
    const score = Module.HEAPF32[score_ptr >> 2];

    Module._free(act_ptr);
    Module._free(score_ptr);

    console.log(`WASM minimax 結果: (${row}, ${col}), score=${score}`);
    return { score,row, col };
}


async function calculate_CPU(lv,current_color,white,black){
    calculating_ele.style.display = "block";
    //console.log("block");
    let act_str,_;
    let row,col;
    //console.log(current_color,white,black);
    if(lv==0){
        const legals = get_legal_square(current_color,white,black).toJs();
        const act = legals[Math.floor(Math.random()*legals.length)];
        act_str = col_letters[7-act[1]] + ((7-act[0]+1));
    }else{
        if(current_color=="white"){
            //const result = minimax(white,black,Number(lv),alpha=-Infinity,beta=Infinity,maximizing_player=true).toJs();
            const result2 = await runMinimax(white,black,lv,1);
            console.log(result2);
            console.log("white");
            row = Number(result2.row);
            col = Number(result2.col);
            act_str = convert_act_bit2str([row,col]);
                /* console.log(result2);
                row = Number(result2.row);
                col = Number(result2.col);
                console.log(row,col);
                act_str = convert_act_bit2str([row,col]);
                console.log(act_str); */
                //const place = act_str;
    
            //[_,act_str] = result2;
            //[_,act_str] = result;
        }else{
            const result2 = await runMinimax(white,black,lv,-1);
            console.log(result2);
            console.log("black");
            row = Number(result2.row);
            col = Number(result2.col);
            act_str = convert_act_bit2str([row,col]);
            /* const result = minimax(black,white,Number(lv),alpha=-Infinity,beta=Infinity,maximizing_player=true).toJs();
            console.log("black");
            [_,act_str] = result; */
            
            //[_,act_str] = result;
        }
        
    }
    const place = act_str; 
    record += place;
    let flips;
    const result = identify_flip_stone(current_color,current_white,current_black,place,2).toJs();
    [current_white,current_black,flips] = result;
    current_white = BigInt(current_white);
    current_black = BigInt(current_black);
    //console.log("none");
    calculating_ele.style.display = "none";
    //console.log("flips:",flips)
    update_turn(current_white,current_black,act_str,flips);
    //console.log(act_str);
    
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
        if (!currentUserUid) {
            signInAnonymouslyOnce().then(() => {
                updateLevelSelectionUI(); // サインイン後にUIを更新
            });
        } else {
            updateLevelSelectionUI(); // 既にサインイン済みならすぐにUIを更新
        }
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

async function updateLevelSelectionUI() {
    const bestWins = await fetchUserBestWins(); // ユーザーの勝利記録を取得

    if (bestWins) {
        for (let level = 0; level <= 9; level++) {
            const levelWonField = `level_${level}_won_at`;
            //const levelElement = document.getElementById(`cpu_level_${level}_selector`); // 例: レベル選択ボタンのID
            
            if (true) {
                if (bestWins[levelWonField]) {
                    // このレベルに勝ったことがある場合
                    //levelElement.classList.add('level-won'); // 勝ったことを示すCSSクラスを追加
                    console.log(`Lv.${level} に勝利済み (${bestWins[levelWonField].toDate().toLocaleString()})`); // ツールチップに日時表示
                    document.getElementById(`lv${level}`).querySelector(".oukan").classList.remove("not_cleard");
                    // 例: チェックマークアイコンを表示する要素を追加
                    // const checkmark = document.createElement('span');
                    // checkmark.textContent = ' ✅';
                    // levelElement.appendChild(checkmark);
                } else {
                    // まだこのレベルに勝ったことがない場合
                    console.log("まだ勝ってない");
                    //levelElement.title = `Lv.${level} は未勝利`;
                }
            }
        }
        let level = 85;
        const levelWonField = `level_${level}_won_at`;
        if (true) {
                if (bestWins[levelWonField]) {
                    // このレベルに勝ったことがある場合
                    //levelElement.classList.add('level-won'); // 勝ったことを示すCSSクラスを追加
                    console.log(`Lv.${8.5} に勝利済み (${bestWins[levelWonField].toDate().toLocaleString()})`); // ツールチップに日時表示
                    document.getElementById(`lv${8.5}`).querySelector(".oukan").classList.remove("not_cleard");
                    // 例: チェックマークアイコンを表示する要素を追加
                    // const checkmark = document.createElement('span');
                    // checkmark.textContent = ' ✅';
                    // levelElement.appendChild(checkmark);
                } else {
                    // まだこのレベルに勝ったことがない場合
                    console.log("まだ勝ってない");
                    //levelElement.title = `Lv.${level} は未勝利`;
                }
            }
    }
    // console.log("レベル選択UIの更新が完了しました。");
}

window.start_up = start_up;
console.log("main.js: window.start_up を設定しました。現在の値:", typeof window.start_up);


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
        update_turn(current_white,current_black,place,flips);
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
    updateLevelSelectionUI();
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
    under_board.style.transform  = `translateY(-${(offset_Y-200)}px)`;

    if(scale!==1){
        let top = real_height + 30+30+30
        if(top<=670){top+=100}
        calculating_ele.style.top = `${top}px`;
        whitch_color.style.top = `${top}px`;
        console.log(top);
        under_board.style.transform  = `translateY(-${(offset_Y-300)}px)`;
    }else{
        calculating_ele.style.top = `0`;
        whitch_color.style.top = `0`;
        under_board.style.transform  = `translateY(-${(offset_Y)}px)`;
    }

    const pageHeight = document.documentElement.scrollHeight;
    over_wrap.style.height = `${pageHeight}px`;
}

window.addEventListener("load", scaleToFit);
window.addEventListener("resize", scaleToFit);

document.getElementById("share-btn").addEventListener("click", () => {
    const white_num = countBits(current_white);
    const black_num = countBits(current_black);
    let winner;
    if(pl_color==1){
        winner = 
            black_num > white_num ? `黒(CPU Lv${cpu_LV})に敗北...` :
            white_num > black_num ? `黒(CPU Lv${cpu_LV})に勝利！！` :
            `黒(CPU${cpu_LV})と引き分け！`;
    }else if(pl_color==-1){
        winner = 
            black_num > white_num ? `白(CPU Lv${cpu_LV})に勝利！！` :
            white_num > black_num ? `白(CPU Lv${cpu_LV})に敗北...` :
            `黒(CPU${cpu_LV})と引き分け！`;
    }else if(pl_color==0){
        winner = 
            black_num > white_num ? `黒が勝利！！` :
            white_num > black_num ? `白が勝利！！` :
            `引き分け！`;
    }
    const text = `●オセロ対戦結果◯\n黒：${black_num}枚　白：${white_num}枚で\n${winner}\n@e_Coach_AI`;
    const hashtags = "オセロ\n,e_Coach_AI";
    const url = "\nhttps://e-coach-ai.com/play_othello.html\n";
    const tweetUrl =
    `https://twitter.com/intent/tweet?text=${encodeURIComponent(text)}&hashtags=${encodeURIComponent(hashtags)}&url=${encodeURIComponent(url)}`;
    window.open(tweetUrl, "_blank");
})







console.log("main.js: スクリプトの実行が完了しました。");


