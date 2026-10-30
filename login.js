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
const database = firebase.database();
const functions = firebase.functions(); // ここで firebase.functions() を呼び出してインスタンスを取得
const firestoreClient = firebase.firestore();
const provider = new firebase.auth.GoogleAuthProvider();
const GoogleAuthProviderClass = firebase.auth.GoogleAuthProvider; 

const loginStatusElement = document.getElementById('login-status');

// ログイン後のリダイレクト先 (例: メインのゲームページ)
const redirectAfterLogin = '../play_othello.html'; // または '/'
let exist_signin = false;


// ログイン状態の変化を監視
auth.onAuthStateChanged(async user => {
    if (user) {
        // ユーザーがログインしている
        if (user.isAnonymous) {
            // 匿名ユーザーの場合は、ログインページに留まり、Googleログインを促す
            //console.log("ユーザーは匿名でログイン済みです。ログインページに留まります。");
            //loginStatusElement.textContent = 'ゲストとしてログイン中。Googleアカウントで進捗を保存できます。';
            // ここで匿名サインインボタンを非表示にするなどしても良い
            //document.getElementById('anonymous-signin').style.display = 'none';

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
        //document.getElementById('google-signin').style.display = '';
        //document.getElementById('anonymous-signin').style.display = '';
        currentUserUid = null;
        console.log("onAuthStateChanged: ユーザーはログアウト状態です。自動匿名サインインを試みます。");
        if(!exist_signin){
            // ★★★ ログアウト状態の場合のみ、匿名サインインを実行 ★★★
            try {
                const userCredential = await auth.signInAnonymously();
                currentUserUid = userCredential.user.uid;
                console.log("onAuthStateChanged: 匿名ユーザーとしてサインイン成功！UID:", currentUserUid);

                // 匿名サインイン後も、Firestore の最終ログイン日時を記録
                try {
                    const userRef = firestoreClient.collection('users').doc(currentUserUid);
                    await userRef.set({
                        lastLoginAt: firebase.firestore.FieldValue.serverTimestamp()
                    }, { merge: true });
                    console.log(`Firestore: 匿名ユーザー ${currentUserUid} の最終ログイン日時を更新しました。`);
                } catch (firestoreError) {
                    console.error("Firestore: 匿名ユーザーの最終ログイン日時更新中にエラーが発生しました:", firestoreError);
                }

                // 匿名ユーザーとしてログイン後、UIを更新
                //updateUserProfileUI(userCredential.user);
                //initializeGameWithUser(currentUserUid); // ゲーム初期化

            } catch (error) {
                console.error("onAuthStateChanged: 匿名サインイン中にエラーが発生しました:", error);
            }
        }else{
            console.log("既存ユーザー処理のため匿名サインイン処理を取り消します。")
        }
    }
});

// Googleサインインボタンのイベントリスナー
document.getElementById('gsi-material-button').addEventListener('click', async () => {
    loginStatusElement.textContent = 'Googleでサインイン中...';
    try {
        // 既存の匿名ユーザーがいれば、Googleアカウントにリンクする
        const anonymousUser = auth.currentUser;
        let userCredential;
        if (anonymousUser && anonymousUser.isAnonymous) {
            console.log("匿名あり: 匿名アカウントにGoogleアカウントをリンクを試みます...");
            
            // ★linkWithPopup の結果を await で受け取り、try-catch でエラー処理を集中させる
            userCredential = await anonymousUser.linkWithPopup(provider); 
            
            console.log("匿名アカウントがGoogleアカウントにリンクされました。");
            console.log("認証成功。メインページへリダイレクトします。");
            //window.location.href = redirectAfterLogin; // リンク成功ならリダイレクト
            console.log("linkWithPopup 成功。userCredential:", userCredential); // ★これを全体的に見たい
            console.log("linkWithPopup 成功。userCredential.user.uid:", userCredential.user.uid);
            console.log("linkWithPopup 成功。userCredential.user.isAnonymous:", userCredential.user.isAnonymous);
            console.log("linkWithPopup 成功。auth.currentUser.uid:", auth.currentUser.uid);
            console.log("linkWithPopup 成功。auth.currentUser.isAnonymous:", auth.currentUser.isAnonymous);
            console.log("providerData:", userCredential.user.providerData);
            
        } else {
            // 匿名でない場合は通常のサインイン（この場合は link ではなく signIn）
            console.log("匿名なし: 通常のGoogleサインインを試みます...");
            userCredential = await auth.signInWithPopup(provider);
            console.log("認証成功。メインページへリダイレクトします。");
            //window.location.href = redirectAfterLogin; // サインイン成功ならリダイレクト
        }
        if (userCredential.user && !userCredential.user.isAnonymous) {
            const user = userCredential.user; // 認証後のユーザーオブジェクト

            // providerData に情報があることを確認
            if (user.providerData && user.providerData.length > 0) {
                const googleProfile = user.providerData.find(p => p.providerId === 'google.com');

                if (googleProfile) {
                    const updates = {};
                    if (!user.displayName && googleProfile.displayName) { // トップレベルの displayName が null かつ providerData にあれば
                        updates.displayName = googleProfile.displayName;
                        console.log("gsi-click: displayName を providerData から更新:", googleProfile.displayName);
                    }
                    if (!user.photoURL && googleProfile.photoURL) { // トップレベルの photoURL が null かつ providerData にあれば
                        updates.photoURL = googleProfile.photoURL;
                        console.log("gsi-click: photoURL を providerData から更新:", googleProfile.photoURL);
                    }

                    if (Object.keys(updates).length > 0) {
                        await user.updateProfile(updates); // プロフィールを更新
                        console.log("gsi-click: ユーザープロフィールを更新しました。");
                        // updateProfile 後は currentUser の情報が更新される
                        // ここで `await auth.currentUser.reload();` も不要なはず
                    }
                    const currentUser = auth.currentUser;
                    const userRef = firestoreClient.collection('users').doc(currentUser.uid);
                    await userRef.set({
                        displayName: updates.displayName, // 必ず新しい名前で更新 (空白チェック済み)
                        photoURL: updates.photoURL,   // photoURLは、ファイル選択がなければ元のURLのまま、あれば新しいURL
                        updatedAt: firebase.firestore.FieldValue.serverTimestamp()
                    }, { merge: true });
                }
            }
            
            // プロフィール更新後、メインページへリダイレクト
            console.log("gsi-click: 認証成功 (匿名ではない)。メインページへリダイレクトします。");
            window.location.href = redirectAfterLogin;

        }
        /* if (userCredential.user && !userCredential.user.isAnonymous) {
            console.log("認証成功。メインページへリダイレクトします。");
            window.location.href = redirectAfterLogin;
        } */
        // 認証成功後は onAuthStateChanged でリダイレクトされる
    }  catch (error) {
        console.log("Googleサインイン中にエラーが発生しました:", error); // エラーはここでキャッチ
        if (error.code === 'auth/popup-closed-by-user') {
            loginStatusElement.textContent = 'サインインがキャンセルされました。';
        } else if (error.code === 'auth/credential-already-in-use') {
            // ★最も重要なエラー処理
            loginStatusElement.textContent = '登録済みアカウントのログイン処理に移行します。';
            exist_signin = true;
            try {
                // ★★★ 現在のログインユーザーをログアウトさせる ★★★
                // (通常、匿名ユーザーがログインしているはずなので、そのセッションを破棄)
                if (auth.currentUser) {
                    await auth.signOut();
                    console.log("gsi-click: 現在のユーザーをログアウトしました。");
                    
                }

                // エラーオブジェクトから認証情報を取得
                // provider.credentialFromError は GoogleAuthProvider の静的メソッド
                const credentialFromError = GoogleAuthProviderClass.credentialFromError(error);
                
                // 取得した認証情報を使って既存アカウントにログイン
                const result = await auth.signInWithCredential(credentialFromError);
                console.log("gsi-click: 既存のGoogleアカウントでログイン成功:", result.user.uid);
                loginStatusElement.textContent = '既存アカウントでログインしました！';
                window.location.href = redirectAfterLogin; // ログイン成功後にリダイレクト

            } catch (reauthError) {
                console.error("gsi-click: 既存アカウントでの再ログイン中にエラー:", reauthError);
                loginStatusElement.textContent = `既存アカウントへのログイン失敗: ${reauthError.message}`;
            }
            /* const cred = provider.credentialFromError(error);
            userCredential = await auth.signInWithCredential(cred); */
            
            //console.warn("auth/credential-already-in-use エラー: 現在の匿名アカウントは、このGoogleアカウントと紐付けできません。");
            // ここでユーザーに、既存のGoogleアカウントでログインするか、
            // 別のGoogleアカウントを使うか、といった選択肢を提示することができます。
            // 例えば、既存のGoogleアカウントでログインするよう促し、そのアカウントに現在の匿名データを移行する方法など。
            // 複雑になるので、まずはエラーメッセージ表示で良いでしょう。
        } else {
            loginStatusElement.textContent = `エラー: ${error.message}`;
        }
    }
});

// 匿名サインインボタンのイベントリスナー
/* document.getElementById('anonymous-signin').addEventListener('click', async () => {
    loginStatusElement.textContent = 'ゲストとしてサインイン中...';
    try {
        await auth.signInAnonymously();
        // 認証成功後は onAuthStateChanged でリダイレクトされる
    } catch (error) {
        console.error("匿名サインイン中にエラーが発生しました:", error);
        loginStatusElement.textContent = `エラー: ${error.message}`;
    }
}); */