# opensky flight dataset from zenodo

## source
https://zenodo.org/records/7923702



## directions
Download the datasets from link above, and then unzip them using
``` bash
gunzip *.csv.gz
```
copy `parse.py` to the folder contains the dataset to generate a new csv file which contains the following information
- callsign: the identifier of the flight displayed on ATC screens (usually the first three letters are reserved for an airline: AFR for Air France, DLH for Lufthansa, etc.)
- origin: a four letter code for the origin airport of the flight (when available);
- destination: a four letter code for the destination airport of the flight (when available);
- firstseen: the UTC timestamp of the first message received by the OpenSky Network;
- lastseen: the UTC timestamp of the last message received by the OpenSky Network;
- day: the UTC day of the last message received by the OpenSky Network;
