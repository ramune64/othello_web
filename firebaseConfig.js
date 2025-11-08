console.log("main.js: Firebase初期化開始。");
// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics";
import { getDatabase, ref, onValue, runTransaction, get, set } from 'firebase/database'; // Realtime DBの関数をインポート

// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
// For Firebase JS SDK v7.20.0 and later, measurementId is optional
const firebaseConfig = {
apiKey: "AIzaSyCQY-RCNAOKjvp8ItF4SJOr3iCgNjZSrGM",
authDomain: "e-coach-ai.firebaseapp.com",
projectId: "e-coach-ai",
storageBucket: "e-coach-ai.firebasestorage.app",
messagingSenderId: "250769341822",
appId: "1:250769341822:web:5d49eff013adeef941c72f",
measurementId: "G-VJJVMNV2HX"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);

export const database = getDatabase(app);
console.log("main.js: Firebase初期化後。");
window.database = database;
console.log("window.database取得完了");