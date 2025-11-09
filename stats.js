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
//const analytics = firebase.analytics(); // firebase-analytics-compat.js を読み込むと使える
//const database = firebase.database(); // firebase-database-compat.js を読み込むと使える
const functions = firebase.functions(); // ここで firebase.functions() を呼び出してインスタンスを取得
//const firestoreClient = firebase.firestore();



const getHistoricalCpuStatsCallable = functions.httpsCallable('getHistoricalCpuStats');
let currentHistoricalData = null; // Function から取得した生データを保持
let myChart = null; // Chart.js の単一インスタンスを保持




async function create_graph(data,selectedItems) {
    if(myChart){
        myChart.destroy();
    }
    document.querySelector('.chart-inner-wrapper').innerHTML = '<div class="chart-container"><canvas id="combinedChart"></canvas></div>';
            
    /* if (!data || Object.keys(data).length === 0 || selectedItems.length === 0) {
        document.getElementById('error').innerText = '表示するデータがないか、項目が選択されていません。';
        document.getElementById('error').style.display = 'block';
        return;
    } */
    const levels = [0,1,2,3,4,5,6,7,8,85,9];
    const levelLabels = [
        "LV.0","LV.1","LV.2","LV.3","LV.4",
        "LV.5","LV.6","LV.7","LV.8","LV.8.5","LV.9"
    ];

    // -----------------------------
    // dataset 3つにまとめる
    // -----------------------------
    const datasets = [];

    if (/* selectedItems.includes("winRate") */true) {
        datasets.push({
            label: "Winning rate",
            data: levels.map(lv => data[lv].winRate),
            backgroundColor: "rgba(75,192,192,0.6)",
            borderColor: "rgba(75,192,192,1)",
            borderWidth: 1,
            yAxisID: "y-left"
        });
    }

    if (/* selectedItems.includes("totalWins") */true) {
        datasets.push({
            label: "Number of wins",
            data: levels.map(lv => data[lv].totalWins),
            backgroundColor: "rgba(54,162,235,0.6)",
            borderColor: "rgba(54,162,235,1)",
            borderWidth: 1,
            yAxisID: "y-right"
        });
    }

    if (/* selectedItems.includes("totalGames") */true) {
        datasets.push({
            label: "Number of matches",
            data: levels.map(lv => data[lv].totalGames),
            backgroundColor: "rgba(255,159,64,0.6)",
            borderColor: "rgba(255,159,64,1)",
            borderWidth: 1,
            yAxisID: "y-right"
        });
    }
    const ctx = document.getElementById('combinedChart').getContext('2d');
    myChart = new Chart(ctx, {
        type: "bar",
        data: {
            labels: levelLabels, // ← 横軸 11 レベル
            datasets: datasets
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                "y-left": {
                    type: "linear",
                    position: "left",
                    beginAtZero: true,
                    max: 100,
                    title: {
                        display: true,
                        text: "Winning rate (%)"
                    },
                    grid: {
                        drawOnChartArea: false, // 右軸と重ならないように
                    },
                },
                "y-right": {
                    type: "linear",
                    position: "right",
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: "number of times"
                    },
                    ticks: {
                        callback: v => Number.isInteger(v) ? v : ""
                    }
                },
                
            },
            plugins: {
                title: {
                    display: true,
                    text:
                        `Statistics by CPU level (${document.getElementById('periodSelect').selectedOptions[0].text})`
                },
                tooltip: {
                    callbacks: {
                        label: function (ctx) {
                            if (ctx.dataset.label === "Winning rate") {
                                return `${ctx.dataset.label}: ${ctx.parsed.y.toFixed(2)}%`;
                            }
                            return `${ctx.dataset.label}: ${ctx.parsed.y}`;
                        }
                    }
                }
            }
        }
    });
    toggleDatasetVisibility("winRate", selectedItems.includes("winRate"));
    toggleDatasetVisibility("totalWins", selectedItems.includes("totalWins"));
    toggleDatasetVisibility("totalGames", selectedItems.includes("totalGames"));
    document.getElementById('loading').style.display = 'none';

}

// UIの状態を読み込み、グラフを再描画する
async function updateGraph() {
    document.getElementById('loading').style.display = 'block';
    document.getElementById('error').style.display = 'none';

    const selectedPeriod = parseInt(document.getElementById('periodSelect').value);
    const selectedItems = Array.from(document.querySelectorAll('.control-group input[type="checkbox"]:checked'))
                                .map(cb => cb.value);

    /* if (selectedItems.length === 0) {
        document.getElementById('loading').style.display = 'none';
        document.getElementById('error').innerText = '表示する項目を少なくとも1つ選択してください。';
        document.getElementById('error').style.display = 'block';
        return;
    } */

    try {
        // Cloud Function を呼び出す (選択された期間を渡す)
        const result = await getHistoricalCpuStatsCallable({ daysAgo: selectedPeriod });
        currentHistoricalData = result.data.aggregated; // 集計済みデータだけを使う

        create_graph(currentHistoricalData, selectedItems); // ★関数名を変更
    } catch (error) {
        console.error("統計データ取得中にエラー:", error);
        document.getElementById('loading').style.display = 'none';
        document.getElementById('error').innerText = `データの読み込み中にエラーが発生しました: ${error.message}`;
        document.getElementById('error').style.display = 'block';
    }
}

// イベントリスナーのセットアップ
document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('periodSelect').addEventListener('change', updateGraph);
    document.querySelectorAll('.control-group input[type="checkbox"]').forEach((checkbox, idx) => {
        checkbox.addEventListener('change', () => {
            toggleDatasetVisibility(checkbox.value, checkbox.checked);
            });
        });
    fetchStatsAndDrawInitial(); // 初期表示用の関数
});


async function fetchStatsAndDrawInitial() {
    document.getElementById('loading').style.display = 'block';
    document.getElementById('error').style.display = 'none';

    const initialPeriod = parseInt(document.getElementById('periodSelect').value);
    const initialItems = Array.from(document.querySelectorAll('.control-group input[type="checkbox"]:checked'))
                                .map(cb => cb.value);
    
    try {
        const result = await getHistoricalCpuStatsCallable({ daysAgo: initialPeriod });
        currentHistoricalData = result.data.aggregated;
        //console.log(currentHistoricalData);

        if (Object.keys(currentHistoricalData).length === 0) {
            document.getElementById('loading').style.display = 'none';
            document.getElementById('error').innerText = '表示するデータがありませんでした。';
            document.getElementById('error').style.display = 'block';
        } else {
            create_graph(currentHistoricalData, initialItems); // ★関数名を変更
        }
    } catch (error) {
        console.error("初期統計データ取得中にエラー:", error);
        document.getElementById('loading').style.display = 'none';
        document.getElementById('error').innerText = `データの初期読み込み中にエラーが発生しました: ${error.message}`;
        document.getElementById('error').style.display = 'block';
    }
}

function toggleDatasetVisibility(key, isChecked) {
    //console.log(key);
    //console.log(isChecked);
    if (!myChart) return;

    const labelMap = {
        winRate: "Winning rate",
        totalWins: "Number of wins",
        totalGames: "Number of matches"
    };

    const targetLabel = labelMap[key];

    myChart.data.datasets.forEach(ds => {
        if (ds.label === targetLabel) {
            ds.hidden = !isChecked;  // ← これでラベルクリックと同じ動きになる
        }
    });

    myChart.update(); // ← アニメーション付きで反映
}