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

const app = firebase.initializeApp(selectedFirebaseConfig);
const auth = firebase.auth();
const provider = new firebase.auth.GoogleAuthProvider();

const loginStatusElement = document.getElementById('login-status');

// ログイン後のリダイレクト先 (例: メインのゲームページ)
const redirectAfterLogin = '../play_othello.html'; // または '/'

// ログイン状態の変化を監視
auth.onAuthStateChanged(user => {
    if (user) {
        // ユーザーがログインしている
        if (user.isAnonymous) {
            // 匿名ユーザーの場合は、ログインページに留まり、Googleログインを促す
            /* console.log("ユーザーは匿名でログイン済みです。ログインページに留まります。");
            loginStatusElement.textContent = 'ゲストとしてログイン中。Googleアカウントで進捗を保存できます。';
            // ここで匿名サインインボタンを非表示にするなどしても良い
            document.getElementById('anonymous-signin').style.display = 'none'; */

        } else {
            // 匿名ではない（Googleアカウントなどで）ログイン済みユーザーはリダイレクト
            console.log("ユーザーはGoogleアカウントなどでログイン済み:", user.uid);
            window.location.href = redirectAfterLogin;
        }
    } else {
        // ログアウト状態の場合
        console.log("ユーザーはログアウト状態です。");
        loginStatusElement.textContent = '';
        // ログアウト状態なので、両方のボタンを表示する
        document.getElementById('google-signin').style.display = '';
        document.getElementById('anonymous-signin').style.display = '';
    }
});

// Googleサインインボタンのイベントリスナー
document.getElementById('gsi-material-button').addEventListener('click', async () => {
    loginStatusElement.textContent = 'Googleでログイン中...';
            try {
                // 既に匿名ユーザーがログインしている場合は、一旦ログアウトさせてからログインを試みる
                // (login.html から直接遷移するわけではないので、通常は匿名ユーザーはいないはずだが、念のため)
                if (auth.currentUser) {
                    await auth.signOut();
                }
                
                // 通常のGoogleサインイン（ここでは新規登録の可能性は低い）
                await auth.signInWithPopup(provider);
                console.log("Googleログイン成功！");
                // 成功したら onAuthStateChanged でリダイレクトされる
            } catch (error) {
                console.error("Googleログイン中にエラーが発生しました:", error);
                if (error.code === 'auth/popup-closed-by-user') {
                    loginStatusElement.textContent = 'ログインがキャンセルされました。';
                } else {
                    loginStatusElement.textContent = `エラー: ${error.message}`;
                }
            }
});