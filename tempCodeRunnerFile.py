fitness_col = df.columns[i]

    top_fitness = df[fitness_col].iloc[2::5].reset_index(drop=True)

    rolling_mean = top_fitness.rolling(window=100).mean()

    # プロット
    plt.figure(figsize=(12,6))
    plt.plot(top_fitness, label=f"{fitness_col} (raw)")   # 薄く描画
    plt.plot(rolling_mean, label="100-gen moving average", color="red", linewidth=2,alpha = 0.6)
    plt.xlabel("Generation")
    plt.ylabel("Fitness")
    plt.legend()
    plt.savefig(f"fig_{fitness_col}.png", dpi=300, bbox_inches="tight")
    #plt.show()