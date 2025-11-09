/**
 * Import function triggers from their respective submodules:
 *
 * const {onCall} = require("firebase-functions/v2/https");
 * const {onDocumentWritten} = require("firebase-functions/v2/firestore");
 *
 * See a full list of supported triggers at https://firebase.google.com/docs/functions
 */
const {setGlobalOptions} = require("firebase-functions/v2");
// ★setGlobalOptions は v2 からインポート
const {onCall, HttpsError} = require("firebase-functions/v2/https");
// ★onCall と HttpsError を v2/https からインポート
const {onSchedule} = require("firebase-functions/v2/scheduler");
const {defineSecret} = require("firebase-functions/params");

// Secret ManagerからOAuth 1.0a のキーを定義
const TWITTER_APP_KEY = defineSecret("TWITTER_API_KEY");
// あなたの API Key
const TWITTER_APP_SECRET = defineSecret("TWITTER_API_KEY_SECRET");
// あなたの API Key Secret
const TWITTER_ACCESS_TOKEN = defineSecret("TWITTER-ACCESS-TOKEN");
// あなたの Access Token (OAuth 1.0a)
const TWITTER_ACCESS_SECRET = defineSecret("ACCESS-TOKEN-SECRET");
const PROD_PROJECT_ID = "e-coach-ai";
// あなたの Access Token Secret (OAuth 1.0a)

// ★追加: 2nd Gen Scheduler
// const {onRequest} = require("firebase-functions/https");
// const logger = require("firebase-functions/logger");

// For cost control, you can set the maximum number of containers that can be
// running at the same time. This helps mitigate the impact of unexpected
// traffic spikes by instead downgrading performance. This limit is a
// per-function limit. You can override the limit for each function using the
// `maxInstances` option in the function's options, e.g.
// `onRequest({ maxInstances: 5 }, (req, res) => { ... })`.
// NOTE: setGlobalOptions does not apply to functions using the v1 API. V1
// functions should each use functions.runWith({ maxInstances: 10 }) instead.
// In the v1 API, each function can only serve one request per container, so
// this will be the maximum concurrent request count.


// Create and deploy your first functions
// https://firebase.google.com/docs/functions/get-started

// exports.helloWorld = onRequest((request, response) => {
//   logger.info("Hello logs!", {structuredData: true});
//   response.send("Hello from Firebase!");
// });

// Firebase Admin SDK をインポート。これにより、セキュリティルールをバイパスしてDBにアクセスできます。
const admin = require("firebase-admin");

// Firebase Functions のモジュールをインポート
// const functions = require("firebase-functions");
const {TwitterApi} = require("twitter-api-v2");
// Firebase Admin SDK を初期化
// Firebaseプロジェクトにデプロイされると自動的にプロジェクトの認証情報を使用します
admin.initializeApp();

// Realtime Database のインスタンスを取得
const db = admin.database();
const firestore = admin.firestore(); // Cloud Firestore を追加
setGlobalOptions({maxInstances: 10});
/**
 * 日本時間 (JST) で今日の日付を 'YYYY-MM-DD' 形式で取得します。
 * クライアントのローカルタイムゾーンに関わらず、常にJSTで処理します。
 * @param {Date} [date=new Date()] 対象の日付オブジェクト。デフォルトは現在時刻。
 * @return {string} 'YYYY-MM-DD' 形式のJST日付文字列。 // ★修正: @returns を追加
 */
function getJSTDateString(date = new Date()) {
  // Intl.DateTimeFormat を使用して、指定したタイムゾーンで日付をフォーマットします。
  // ほとんどのモダンブラウザは 'Asia/Tokyo' タイムゾーンをサポートしています。
  const formatter = new Intl.DateTimeFormat("ja-JP", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    timeZone: "Asia/Tokyo",
  });
    // formatToParts を使用して、日付の各部分を抽出して 'YYYY-MM-DD' 形式を構築します。
  const parts = formatter.formatToParts(date);
  const year = parts.find((p) => p.type === "year")?.value;
  const month = parts.find((p) => p.type === "month")?.value;
  const day = parts.find((p) => p.type === "day")?.value;
  return `${year}-${month}-${day}`;
}
/**
 * 日本時間 (JST) で今日の日付を 'YYYY-MM-DD' 形式で取得します。
 * クライアントのローカルタイムゾーンに関わらず、常にJSTで処理します。
 * @param {int} [daysAgo] 対象の日付オブジェクト。デフォルトは現在時刻。
 * @return {string} 'YYYY-MM-DD' 形式のJST日付文字列。 // ★修正: @returns を追加
 */
function getJSTPastDateString(daysAgo) {
  // JSTの今日の日付文字列を取得し、それを元にDateオブジェクトを作成することで、
  // クライアントのローカルタイムゾーンの影響を受けずにJSTでの日数計算を可能にします。
  const jstTodayString = getJSTDateString(new Date());
  const [year, month, day] = jstTodayString.split("-").map(Number);
  // Date.UTC を使用して、JSTの日付をUTCとして設定します (例: 2023-11-20 00:00 UTC)
  const jstDate = new Date(Date.UTC(year, month - 1, day));

  // UTCの日付から指定日数分を減算します
  jstDate.setUTCDate(jstDate.getUTCDate() - daysAgo);

  // 減算後の日付を再びJSTとしてフォーマットします
  const formatter = new Intl.DateTimeFormat("ja-JP", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    timeZone: "Asia/Tokyo",
  });
  const parts = formatter.formatToParts(jstDate);
  const pastYear = parts.find((p) => p.type === "year")?.value;
  const pastMonth = parts.find((p) => p.type === "month")?.value;
  const pastDay = parts.find((p) => p.type === "day")?.value;
  return `${pastYear}-${pastMonth}-${pastDay}`;
}
/**
 * オセロの勝敗結果を検証し、Realtime Databaseに記録するCallable Cloud Function。
 * クライアントからは firebase.functions().httpsCallable('recordOthelloResult') で呼び出されます。
 *
 * @param data { object } クライアントから送信されるデータ。
 *   - cpuLevel: number (CPUレベル、0-9)
 *   - playerWon: boolean (プレイヤーが勝った場合true、負けた場合false)
 * @param context { functions.https.CallableContext } 呼び出しのコンテキスト情報。
 *   - auth: Firebase Authentication のユーザー情報を含む（匿名ユーザーも含む）。
 */
exports.recordOthelloResult = onCall(async (request) => {
  // 1. 呼び出し元が認証されているか検証
  console.log("context_check:", request);
  if (!request.auth) {
    // 認証されていない場合はエラーを返す
    throw new HttpsError(
        "unauthenticated",
        "この操作には認証が必要です。",
    );
  }
  const now = new Date(); // Firebaseサーバーの現在時刻 (ミリ秒)
  const nowMs = now.getTime();
  // --- レートリミットのチェック ---
  // rate_limits/{uid}/last_request_timestamp に最終リクエスト時刻を保存
  const uid = request.auth.uid; // 認証済みのユーザーUID（匿名ユーザーのUIDも含む）
  const rateLimitRef = db.ref(`rate_limits/${uid}/last_request_timestamp`);
  const ONE_MINUTE_MS = 60 * 1000; // 1分 (ミリ秒)

  console.log(`UID ${uid} からのゲーム結果受信。`);
  try {
    const rateLimitResult=await rateLimitRef.transaction((currentTimestamp) => {
      console.log("last_request:", currentTimestamp);
      if (currentTimestamp!==null) {
        console.log("now:", nowMs);
        console.log("span:", nowMs - currentTimestamp);
      }
      // currentTimestamp が null の場合、初めてのリクエストなので許可
      // currentTimestamp が null でない場合、最後の送信からの経過時間をチェック
      if (currentTimestamp===null||(nowMs - currentTimestamp)>ONE_MINUTE_MS) {
        // 許可: 現在時刻を新しいタイムスタンプとして記録
        return nowMs;
      } else {
        // 拒否: トランザクションをアボート (undefined を返すことで変更なしを示す)
        return undefined;
      }
    });

    if (!rateLimitResult.committed) { // トランザクションがコミットされなかった場合
      throw new HttpsError(
          "resource-exhausted", // リソースが枯渇していることを示すエラーコード
          "リクエストが多すぎます。1分後に再度お試しください。",
      );
    }
    // ここに到達したということは、レートリミットを通過し、タイムスタンプが更新された
    console.log(`UID ${uid} がレートリミットを通過しました。`);
  } catch (error) {
    // transaction() 自体のエラー（ネットワーク問題など）はここでキャッチ
    if (error instanceof HttpsError) {
      throw error; // HttpsError ならそのまま再スロー
    }
    console.error(`UID ${uid}: レートリミットチェック中にエラーが発生しました:`, error);
    throw new HttpsError(
        "internal",
        "レートリミットチェック中にエラーが発生しました。",
        error.message,
    );
  }
  // --- レートリミットのチェック 終わり ---
  // 2. 入力データの検証
  let {cpuLevel, cpuResult, record} = request.data;
  console.log("data::", request.data);
  if (typeof cpuLevel !== "number" || cpuLevel < 0 || cpuLevel > 9) {
    throw new new HttpsError(
        "invalid-argument",
        "cpuLevel は 0 から 9 までの数値である必要があります。",
    );
  }
  if (cpuLevel==8.5) {
    cpuLevel=85;
  }
  if (-1 > cpuResult || cpuResult > 1) {
    throw new HttpsError(
        "invalid-argument",
        "cpuResult は-1~1である必要があります。",
    );
  }

  // 3. 今日の日付 (JST) を取得
  const todayJST = getJSTDateString();

  // --- Realtime Database の処理 ---
  const dailyStatsRef = admin.database().ref(`daily_cpu_stats/${todayJST}`);

  // ★追加: その日のデータが全く存在しない場合、全レベルの0データを初期化する
  // トランザクションを使って、複数のリクエストが同時に来ても安全に処理
  await dailyStatsRef.transaction((currentData) => {
    if (currentData === null) {
      console.log(`Realtime Database: ${todayJST} の初回対戦。全レベルの0データを初期化します。`);
      const defaultStats = {};
      for (let i = 0; i <= 9; i++) {
        defaultStats[`cpu_level_${i}`] = {wins: 0, losses: 0, draws: 0};
      }
      defaultStats[`cpu_level_85`]={wins: 0, losses: 0, draws: 0};
      // Lv.8.5 も忘れずに

      return defaultStats; // このデフォルトデータで初期化
    } else {
      return undefined; // データが既に存在すれば、何もせず続行 (トランザクションをキャンセル)
    }
  });

  const statsRef = db.ref(`daily_cpu_stats/${todayJST}/cpu_level_${cpuLevel}`);
  let transactionCommitted = false; // トランザクションがコミットされたか追跡
  let firestoreGameRecordRecorded = false; // Firestoreに記録されたか追跡
  let firestoreBestWinRecorded = false; // ベスト勝利記録保存の成功フラグ
  // 4. Realtime Database のトランザクションを使ってデータを更新
  try {
    const rtDbResult = await statsRef.transaction((currentData) => {
      let dataToUpdate;
      if (currentData) {
        dataToUpdate = {...currentData};
      } else {
        dataToUpdate = {wins: 0, losses: 0, draws: 0};
      }

      if (cpuResult === 1) dataToUpdate.wins++;
      else if (cpuResult === -1) dataToUpdate.losses++;
      else if (cpuResult === 0) dataToUpdate.draws++;

      console.log("cpuResult:", cpuResult);
      return dataToUpdate;
    });

    if (rtDbResult.committed) {
      transactionCommitted = true;
      console.log(`UID${uid}:CPUレベル${cpuLevel}の勝敗が正常に更新されました`);
      // return { status: 'success', message: 'ゲーム結果が正常に記録されました。' };
    } else {
      console.log(`UID ${uid}: CPUレベル${cpuLevel}との勝敗更新トランザクションは中断されました。`);
      return {status: "aborted", message: "トランザクションが中断されました。"};
    }

    if (cpuLevel>=8&&record&&cpuResult==-1) {
      console.log(`UID ${uid}: 高レベルCPU敗北の棋譜を記録します (Lv.${cpuLevel})。`);
      try {
        // Cloud Firestore のコレクション 'cpu_game_records' にドキュメントを追加
        // ドキュメントIDは自動生成
        const trueCpuLevel = (cpuLevel === 85) ? 8.5 : cpuLevel;
        await firestore.collection("cpu_game_records").add({
          uid: uid, // 匿名ユーザーのUID
          cpuLevel: trueCpuLevel, // CPUレベル
          timestamp: admin.firestore.FieldValue.serverTimestamp(),
          gameRecord: record, // クライアントから送られてきた棋譜データ
          dateJST: todayJST, // 日本時間の日付
        });
        console.log(`UID ${uid}:棋譜データをFirestoreに記録 (Lv.${trueCpuLevel})。`);
        firestoreGameRecordRecorded = true;
      } catch (firestoreError) {
        console.error(`UID ${uid}: 棋譜データのFirestoreへの記録中にエラー:`, firestoreError);
        // 棋譜記録が失敗しても、勝敗記録は成功しているので、致命的なエラーとはしない
        // クライアントには警告メッセージを返す
        return {
          status: "success_with_warning",
          message: "ゲーム結果は記録されましたが、棋譜の記録中にエラーが発生しました。",
          firestoreError: firestoreError.message,
        };
      }
    }
  } catch (error) {
    console.error(`UID ${uid}: CPUレベル${cpuLevel}との勝敗更新中にエラーが発生しました:`, error);
    throw new HttpsError(
        "internal",
        "データベースの更新中にエラーが発生しました。",
        error.message,
    );
  }
  if (cpuResult==-1) {
    const userBestWinsRef = firestore.collection("user_best_wins").doc(uid);
    const levelFieldName = `level_${cpuLevel}_won_at`;
    try {
      await firestore.runTransaction(async (t) => {
        const doc = await t.get(userBestWinsRef);
        if (!doc.exists) {
          // ドキュメントがまだ存在しない場合、作成
          t.set(userBestWinsRef, {
            [levelFieldName]: admin.firestore.FieldValue.serverTimestamp(),
            lastUpdated: admin.firestore.FieldValue.serverTimestamp(),
          });
        } else {
          // ドキュメントが存在し、まだこのレベルに勝った記録がない場合、追加
          if (!doc.data()[levelFieldName]) {
            t.update(userBestWinsRef, {
              [levelFieldName]: admin.firestore.FieldValue.serverTimestamp(),
              lastUpdated: admin.firestore.FieldValue.serverTimestamp(),
            });
          } else {
            // 既に記録がある場合は何もしない（最初の勝利時刻を保持）
            t.update(userBestWinsRef, {
              lastUpdated: admin.firestore.FieldValue.serverTimestamp(),
              // 最終更新時刻は常に更新
            });
          }
        }
      });
      firestoreBestWinRecorded = true;
      console.log(`UID ${uid}: CPUレベル${cpuLevel}へのベスト勝利記録を更新/確認しました。`);
    } catch (bestWinError) {
      console.error(`UID${uid}:ベスト勝利記録更新中にエラー(Lv.${cpuLevel}):`, bestWinError);
    }
  }
  let status = "success";
  let message = "ゲーム結果が正常に記録されました。";
  if (transactionCommitted &&
    !firestoreGameRecordRecorded&&
    cpuLevel>=8&&cpuResult===-1) {
    status = "success_with_warning";
    message = "ゲーム結果は記録されましたが、棋譜の記録中にエラーが発生しました。";
  } else if (transactionCommitted &&
    !firestoreBestWinRecorded && cpuResult === -1) {
    status = "success_with_warning";
    message = "ゲーム結果は記録されましたが、ベスト勝利記録の更新中にエラーが発生しました。";
  }
  return {status: status, message: message};
});

// 毎日JST 00:01 に実行
exports.cleanUpOldData = onSchedule(
    {
      schedule: "1 0 * * *", // Cron形式
      timeZone: "Asia/Tokyo", // 日本時間でスケジュールを設定
    },
    async (context) => {
      console.log("データクリーンアップ関数が実行されました。");

      // --- 1. まず、32日以上前の累積データを削除 ---
      const todayJST = getJSTDateString();
      const oldThresholdDate = getJSTPastDateString(93); // 日本時間で32日前の日付を取得
      const dailyStatsRootRef = db.ref("daily_cpu_stats");
      const usersCollectionRef = admin.firestore().collection("users");
      // Firestore users コレクション
      const userBestWinsCollectionRef=admin.firestore()
          .collection("user_best_wins");
      // Firestore user_best_wins コレクション
      try {
        const snapshot = await dailyStatsRootRef.orderByKey()
            .endAt(oldThresholdDate).once("value");
        if (snapshot.exists()) {
          const updates = {};
          snapshot.forEach((childSnapshot) => {
            updates[childSnapshot.key] = null; // null を設定することで削除
          });
          await dailyStatsRootRef.update(updates); // ★一括削除
          console.log(`Realtime Database: ${snapshot.numChildren()} 件の古いデータ`+
          `(${oldThresholdDate} 以前) を削除しました。`);
        } else {
          console.log(`Realtime Database:${oldThresholdDate}`+
            `以前の削除対象データは見つかりませんでした。`);
        }
        // --- 2. 今日の日付の0データを存在しなければ作成 ---
        const todayStatsRef = admin.database().
            ref(`daily_cpu_stats/${todayJST}`);
        const todayStatsSnapshot = await todayStatsRef.once("value");

        if (!todayStatsSnapshot.exists()) {
          console.log(`Realtime Database: ${todayJST} のデータが存在しません。0データを作成します。`);
          const defaultStats = {};
          for (let i = 0; i <= 9; i++) {
            defaultStats[`cpu_level_${i}`] = {wins: 0, losses: 0, draws: 0};
          }
          defaultStats[`cpu_level_85`] = {wins: 0, losses: 0, draws: 0};
          // Lv.8.5 も忘れずに

          await todayStatsRef.set(defaultStats); // set で一括書き込み
          console.log(`Realtime Database: ${todayJST} の0データを正常に作成しました。`);
        } else {
          console.log(`Realtime Database: ${todayJST}`+
            `のデータは既に存在します。作成はスキップします。`);
        }
        for (let i = 0; i <= 10; i++) { // ★ループ範囲
          let lv = i;
          if (i === 9) lv = 85; // ★lv を 85 に変更
          else if (i === 10) lv = 9; // ★lv を 9 に変更

          // let defaultStats; // この変数は不要
          const StatsRef =
          db.ref(`daily_cpu_stats/${todayJST}/cpu_level_${lv}`);
          await StatsRef.transaction((currentData) => {
            if (currentData === null) {
              console.log(`Realtime Database: ${todayJST}/cpu_level_${lv} `+
              `のデータが存在しないため、0データを作成します。`);
              return {wins: 0, losses: 0, draws: 0}; // 直接オブジェクトを返す
            } else {
              return undefined; // データが既に存在すれば、何もせず続行 (トランザクションをキャンセル)
            }
          });
        }
        console.log("Firestore: 3ヶ月以上ログインしていないユーザーの初勝利記録をクリーンアップします。");

        // 3ヶ月前のタイムスタンプを計算
        const threeMonthsAgo = new Date();
        threeMonthsAgo.setMonth(threeMonthsAgo.getMonth() - 3);
        const threeMonthsAgoTimestamp =
        admin.firestore.Timestamp.fromDate(threeMonthsAgo);
        // lastLoginAt が threeMonthsAgo より古いユーザーを検索
        const inactiveUsersSnapshot = await usersCollectionRef
            .where("lastLoginAt", "<", threeMonthsAgoTimestamp)
            .get();

        if (inactiveUsersSnapshot.empty) {
          console.log("Firestore: 3ヶ月以上ログインしていないユーザーは見つかりませんでした。");
        } else {
          const batch = admin.firestore().batch(); // バッチ書き込みを使って複数削除を効率化
          let deletedCount = 0;

          inactiveUsersSnapshot.forEach((userDoc) => {
            const userId = userDoc.id;
            console.log(`Firestore: ユーザー ${userId} (最終ログイン:`+
              ` ${userDoc.data().lastLoginAt.toDate().toISOString()}) `+
              `は3ヶ月以上ログインしていません。`);

            // 該当ユーザーの user_best_wins ドキュメントを削除対象に追加
            const userBestWinDocRef = userBestWinsCollectionRef.doc(userId);
            batch.delete(userBestWinDocRef);
            // ★追加: users コレクションのドキュメントも削除対象に追加
            const userProfileDocRef = usersCollectionRef.doc(userId);
            batch.delete(userProfileDocRef); // users コレクションからユーザーのドキュメントも削除
            deletedCount++;
          });

          if (deletedCount > 0) {
            await batch.commit(); // バッチ処理を実行
            console.log(`Firestore: ${deletedCount} 件の初勝利記録を削除しました。`);
          }
        }
      } catch (error) {
        console.error("古いデータのクリーンアップ中にエラーが発生しました:", error);
      }


      return null;
    });

exports.postDailyOthelloStatsToX = onSchedule(
    {
      schedule: "0 7 * * *", // Cron形式
      timeZone: "Asia/Tokyo", // 日本時間でスケジュールを設定
      secrets: [
        TWITTER_APP_KEY,
        TWITTER_APP_SECRET,
        TWITTER_ACCESS_TOKEN,
        TWITTER_ACCESS_SECRET,
      ],
    },
    async (context) => {
      const currentProjectId = admin.app().options.projectId;

      if (currentProjectId !== PROD_PROJECT_ID) {
        console.log(`現在のプロジェクト (${currentProjectId})`+
          `は本番環境ではありません。Twitter投稿をスキップします。`);
        return null;
      }
      // Secret Managerから環境変数としてシークレットにアクセス
      const appKey = TWITTER_APP_KEY.value();
      const appSecret = TWITTER_APP_SECRET.value();
      const accessToken = TWITTER_ACCESS_TOKEN.value();
      const accessSecret = TWITTER_ACCESS_SECRET.value();

      if (!appKey || !appSecret || !accessToken || !accessSecret) {
        console.error("Twitter API 認証情報が不足しています。投稿をスキップします。");
        return null;
      }
      try {
      // OAuth 1.0a User Context でクライアントを初期化
        const client = new TwitterApi({
          appKey: appKey,
          appSecret: appSecret,
          accessToken: accessToken,
          accessSecret: accessSecret,
        });

        // API v2 のエンドポイントにアクセスするためのラッパー
        const twitterV2Client = client.v2;
        // const tweetText = "【テスト投稿】\nAPI使用ポストのテストです。";

        // const todayJST = getJSTDateString();
        const yesterdayJST = getJSTPastDateString(1);
        // 前日の日付を取得 (例: "2023-10-26")
        const statsRef=admin.database().ref(`daily_cpu_stats/${yesterdayJST}`);
        const snapshot = await statsRef.once("value");
        const dailyStats = snapshot.val(); // ここに前日の一日分の統計データが入る
        // 表示用に "YYYY/MM/DD" 形式に変換
        const displayDate = yesterdayJST.replace(/-/g, "/");
        let tweetTextp1=`おはようございます！\n昨日のAIと皆さんの対戦成績です！(Lv.0~Lv.3)`+
        `\n${displayDate}\n\n`;
        let tweetTextp2=`おはようございます！昨日のAIと皆さんの対戦成績です！(Lv.4~Lv.7)`+
        `\n${displayDate}\n\n`;
        let tweetTextp3=`おはようございます！昨日のAIと皆さんの対戦成績です！(Lv.8~Lv.9)`+
        `\n${displayDate}\n\n`;
        const tweetParts = [];
        if (dailyStats) {
          // dailyStats の構造例:
          // {
          //   "cpu_level_0": { "wins": 5, "losses": 2, "draws": 1 },
          //   "cpu_level_1": { "wins": 10, "losses": 3, "draws": 0 },
          //   // ...
          // }

          for (let i = 0; i <= 10; i++) {
            let trueCpuLevel = i;
            let keyNum = i;
            if (i==9) {
              trueCpuLevel = 8.5;
              keyNum = 85;
            } else if (i==10) {
              trueCpuLevel = 9;
              keyNum = 9;
            }
            const levelKey = `cpu_level_${keyNum}`;
            const levelStats=dailyStats[levelKey]||
            {wins: 0, losses: 0, draws: 0};
            // データがないレベルも考慮
            if (i<=3) {
              tweetTextp1+=`vs Lv.${trueCpuLevel}: プレイヤー${levelStats.losses}勝`+
              `${levelStats.wins}敗${levelStats.draws}分\n`;
            } else if (4<=i&&i<=7) {
              tweetTextp2+=`vs Lv.${trueCpuLevel}: プレイヤー${levelStats.losses}勝`+
              `${levelStats.wins}敗${levelStats.draws}分\n`;
            } else {
              tweetTextp3+=`vs Lv.${trueCpuLevel}: プレイヤー${levelStats.losses}勝`+
              `${levelStats.wins}敗${levelStats.draws}分\n`;
            }
          }
          tweetTextp1+="#オセロ \nhttps://e-coach-ai.com/play_othello.html";
          tweetTextp2+="#オセロ \nhttps://e-coach-ai.com/play_othello.html";
          tweetTextp3+="#オセロ \nhttps://e-coach-ai.com/play_othello.html";
          tweetParts.push(tweetTextp1);
          tweetParts.push(tweetTextp2);
          tweetParts.push(tweetTextp3);
        }
        for (const tweetText of tweetParts) {
          if (!tweetText.trim()) {
            console.log("空のツイートをスキップします。");
            continue; // 空のツイートはスキップ
          }

          // ツイート投稿
          const {data: tweetResponse} = await twitterV2Client.tweet(tweetText);

          console.log(`ツイート投稿成功 (ID: ${tweetResponse.id}): ${tweetText}`);

          // ツイート間に少し間隔を空ける (レート制限対策)
          // await new Promise((resolve) => setTimeout(resolve, 3000)); // 3秒間隔
        }
        console.log("X (Twitter) に投稿しました");
        await new Promise((resolve) => setTimeout(resolve, 3000));
      } catch (error) {
        console.error("X (Twitter) への投稿中にエラーが発生しました:", error);
      }
    },
);

exports.getHistoricalCpuStats = onCall(async (request) => {
  // この関数は公開データ（集計結果）を返すため、request.auth (認証) は不要と仮定
  // 必要であれば認証を追加することも可能です。
  const {daysAgo = 30} = request.data || {}; // リクエストデータがなければデフォルト30日

  if (typeof daysAgo !== "number" || daysAgo < 1 || daysAgo > 90) {
    throw new HttpsError("invalid-argument", "daysAgo は1から90までの数値で指定してください。");
  }
  try {
    const dailyStatsRootRef = admin.database().ref("daily_cpu_stats");

    const todayJST = getJSTDateString();
    const startDateJST=getJSTPastDateString(daysAgo); // 30日前の日付（この日以降のデータが必要）

    // Realtime Database から過去30日間のデータを取得
    // 日付キーでソートし、30日前の日付から今日までのデータを取得
    const snapshot = await dailyStatsRootRef
        .orderByKey()
        .startAt(startDateJST)
        .endAt(todayJST)
        .once("value");

    const allHistoricalStats = snapshot.val(); // 過去30日間の全データ
    if (!allHistoricalStats) {
      console.log("getHistoricalCpuStats: 過去30日間のデータは見つかりませんでした。");
      return {}; // データがなければ空オブジェクトを返す
    }

    // --- ここでデータを集計・加工 ---
    // 例: 各CPUレベルの総勝利数、総対戦数などを計算
    const aggregatedStats = {};
    const allLevels = [0, 1, 2, 3, 4, 5, 6, 7, 8, 85, 9];
    // 集計をより柔軟にするため、日ごとのデータも保持する形にする
    const rawDataByLevelAndDate = {};
    // { level: { date: { wins, losses, draws }, ... }, ... }
    for (const level of allLevels) {
      aggregatedStats[level] =
        {totalWins: 0, totalLosses: 0, totalDraws: 0,
          totalGames: 0, winRate: 0};
      rawDataByLevelAndDate[level] = {};
    }
    for (const dateKey in allHistoricalStats) {
      if (Object.prototype.hasOwnProperty.call(allHistoricalStats, dateKey)) {
        const dailyData = allHistoricalStats[dateKey];

        for (const level of allLevels) {
          const levelKey = `cpu_level_${level}`;
          const stats = dailyData[levelKey] || {wins: 0, losses: 0, draws: 0};

          if (!aggregatedStats[level]) {
            aggregatedStats[level] = {totalWins: 0, totalLosses: 0,
              totalDraws: 0, totalGames: 0};
          }
          aggregatedStats[level].totalWins += stats.wins;
          aggregatedStats[level].totalLosses += stats.losses;
          aggregatedStats[level].totalDraws += stats.draws;
          aggregatedStats[level].totalGames += (stats.wins +
            stats.losses + stats.draws);

          rawDataByLevelAndDate[level][dateKey] = stats;
        }
      }
    }

    for (const level in aggregatedStats) {
      if (Object.prototype.hasOwnProperty.call(aggregatedStats, level)) {
        const stats = aggregatedStats[level];
        stats.winRate =
        stats.totalGames > 0 ? (stats.totalWins / stats.totalGames) * 100 : 0;
      }
    }

    console.log("getHistoricalCpuStats: 過去30日間の統計データを集計しました。");
    // 集計済みデータと日ごとの生データを両方返す
    return {aggregated: aggregatedStats, rawByDate: rawDataByLevelAndDate};
  } catch (error) {
    console.error("getHistoricalCpuStats: 統計データ取得中にエラーが発生しました:", error);
    throw new HttpsError("internal", "過去の統計データ取得に失敗しました。", error);
  }
});
