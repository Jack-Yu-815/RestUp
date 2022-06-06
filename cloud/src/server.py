from flask import Flask
from flask import request, render_template
from pathlib import Path
import csv
import pandas as pd
import json
import time

app = Flask(__name__, template_folder="../templates")
record_path = Path("../records")
STARTTIME_PATH = "../records/starttime.csv"
assert record_path.exists()

def format_starttime_index(user_id, session_id):
    return "{:010}".format(int(user_id)) + "+" + "{:010}".format(int(session_id))


def record_start_time(user_id, session_id, file_path: str, time_since_start: int=None):
    """
    Write a row containing [user_id&session_id, start_time] in the .csv file. The start_time is the time when the device is powered on (in miliseconds since the epoch).
    parameters
    ----------
    user_id: str
    session_id: str
    file_path: str
        the path to the csv file that stores the starting time of the hardware in miliseconds
    time_since_start: int
        the time since the device is powered on (in miliseconds)
    """
    df = pd.read_csv(file_path, index_col=0, dtype=str)
    starttime_index = format_starttime_index(user_id, session_id)
    if starttime_index not in df.index:
        with open(file_path, "a") as file:
            writer = csv.writer(file)
            start_time = int(time.time() * 1000) - time_since_start
            writer.writerow([format_starttime_index(user_id, session_id), start_time])


def get_start_time(user_id: str, session_id: str, file_path: str) -> int:
    assert Path(file_path).exists()
    df = pd.read_csv(file_path, index_col=0, dtype=str)
    starttime_index = format_starttime_index(user_id, session_id)
    print(df.index)
    if starttime_index in df.index:
        return int(df.loc[starttime_index, "start time"])
    else:
        raise KeyError(f"index {starttime_index} not defined")


def get_avg(user_id: str, session_id: str) -> tuple:
    session_path = record_path / user_id / (session_id + ".csv")
    assert session_path.exists()
    df = pd.read_csv(str(session_path))
    means = df.mean(0)
    return means


def add_entry(user_id: str, session_id: str, entries: [str]):
    session_path = record_path / user_id / (session_id + ".csv")
    is_new_session = not session_path.exists()
    with session_path.open('a') as file:
        writer = csv.writer(file)
        if is_new_session:
            writer.writerow(["time", "heart rate", "temperature"])
            record_start_time(user_id, session_id, STARTTIME_PATH, int(entries[0]))
        writer.writerow(entries)
        print("entries written to", session_path)


def get_sleep_data(user_id, session_id):
    list1 = []
    session_path = record_path / user_id / (session_id + ".csv")
    is_header = True
    with session_path.open("r") as file:
        reader = csv.reader(file)
        for row in reader:
            if not is_header:
                list1.append([int(row[0]), int(row[1]), float(row[2])])
            else:
                is_header = False

    list1.sort(key=lambda x: x[0])  # sort by timestamp
    return list1


@app.route("/")
def record_datapoint():
    heart_rate = request.args.get("heart_rate")
    temp = request.args.get("temp")
    user_id = request.args.get('user_id')
    session_id = request.args.get('session_id')
    time_since_start = request.args.get('time')

    # check that user exists
    user_path = record_path / user_id
    if not user_path.exists():
        return "unknown user id " + user_id, 403

    session_path = record_path / user_id / (session_id + ".csv")
    # if is a new_session, record its start time
    if not session_path.exists():
        record_start_time(user_id, session_id, STARTTIME_PATH, int(time_since_start))
    timestamp = get_start_time(user_id, session_id, STARTTIME_PATH) + int(time_since_start)  # in miliseconds since epoch
    entries = [timestamp, heart_rate, temp]

    # record data entry
    entries = [str(e) if e != None else '' for e in entries]
    add_entry(user_id, session_id, entries)
    return str(entries)

@app.route("/new_session")
def new_session_id():
    user_id = request.args.get('user_id')
    t = request.args.get('time')
    
    if any(n is None for n in [user_id, t]):
        raise ValueError("missing required URL parameter")
    try:
        recent_id = max(int(p.stem) for p in (record_path / user_id).iterdir())
    except ValueError:
        recent_id = 0
    session_id = str(recent_id + 1)
    return session_id


@app.route("/graph")
def get_graph():
    user_id = request.args.get('user_id')
    session_id = request.args.get('session_id')

    # check required parameters are in the URL
    if any(n is None for n in [user_id, session_id]):
        raise ValueError("missing required URL parameter")

    list1 = get_sleep_data(user_id, session_id)
    means = get_avg(user_id, session_id)
    print("mean:", means)
    print("means.loc[heart rate]:", means.loc["heart rate"])
    analysis = {
            "raw": list1,
            "heard_rate_avg": str(means.loc["heart rate"]),
            "temp_avg": str(round(float(means.loc["temperature"]), 2))
            }
    return render_template("dual_y.html", analysis=json.dumps(analysis))
