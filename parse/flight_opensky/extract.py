import pandas as pd

# List all CSV files in the current directory
csv_files = [
"flightlist_20190101_20190131.csv",
"flightlist_20190201_20190228.csv",
"flightlist_20190301_20190331.csv",
"flightlist_20190401_20190430.csv",
"flightlist_20190501_20190531.csv",
"flightlist_20190601_20190630.csv",
"flightlist_20190701_20190731.csv",
"flightlist_20190801_20190831.csv",
"flightlist_20190901_20190930.csv",
"flightlist_20191001_20191031.csv",
"flightlist_20191101_20191130.csv",
"flightlist_20191201_20191231.csv",
"flightlist_20200101_20200131.csv",
"flightlist_20200201_20200229.csv",
"flightlist_20200301_20200331.csv",
"flightlist_20200401_20200430.csv",
"flightlist_20200501_20200531.csv",
"flightlist_20200601_20200630.csv",
"flightlist_20200701_20200731.csv",
"flightlist_20200801_20200831.csv",
"flightlist_20200901_20200930.csv",
"flightlist_20201001_20201031.csv",
"flightlist_20201101_20201130.csv",
"flightlist_20201201_20201231.csv",
"flightlist_20210101_20210131.csv",
"flightlist_20210201_20210228.csv",
"flightlist_20210301_20210331.csv",
"flightlist_20210401_20210430.csv",
"flightlist_20210501_20210530.csv",
"flightlist_20210601_20210630.csv",
"flightlist_20210701_20210731.csv",
"flightlist_20210801_20210831.csv",
"flightlist_20210901_20210930.csv",
"flightlist_20211001_20211031.csv",
"flightlist_20211101_20211130.csv",
"flightlist_20211201_20211231.csv",
"flightlist_20220101_20220131.csv",
"flightlist_20220201_20220228.csv",
"flightlist_20220301_20220331.csv",
"flightlist_20220401_20220430.csv",  
"flightlist_20220501_20220531.csv",
"flightlist_20220601_20220630.csv",
"flightlist_20220701_20220731.csv",
"flightlist_20220801_20220831.csv",
"flightlist_20220901_20220930.csv",
"flightlist_20221001_20221031.csv",
"flightlist_20221101_20221130.csv",
"flightlist_20221201_20221231.csv"
]

dataframes = []
cols_to_keep = [5, 6, 7, 8]
for i, file in enumerate(csv_files):
    df = pd.read_csv(file, header=0, usecols=cols_to_keep)
    dataframes.append(df)

# Concatenate all dataframes
combined_df = pd.concat(dataframes, ignore_index=True)
# combined_df.columns = ["origin", "destination", "firstseen", "lastseen"]
# Save the result to a new file
combined_df.to_csv("flight_opensky.csv", index=False, header=True)
# combined_df.to_csv("flight_opensky.csv", index=False)


