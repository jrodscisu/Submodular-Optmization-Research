import pandas as pd

df = pd.read_csv('Outputs/outputs.csv')

for b in range(11):
    filtered_all = pd.DataFrame()
    filtered_df = df[df['B'] == b]
    filtered_all = pd.concat([filtered_all, filtered_df])
    filtered_all.to_csv("Outputs/filteredByB" + str(b) + ".csv", index=False)
