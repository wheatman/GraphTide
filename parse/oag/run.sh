    for ((i=1; i<=16; i++)); do
     wget "https://opendata.aminer.cn/dataset/v5_oag_publication_$i.zip" &&
     unzip "v5_oag_publication_$i.zip" &&
     rm "v5_oag_publication_$i.zip" &&
     python "code.py"  "$i" &&
     rm "v5_oag_publication_$i.json"
    done