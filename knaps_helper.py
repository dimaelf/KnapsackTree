#!/usr/bin/python
import os
import time
import pipes
import json

log_file = open("log.txt", "w+")

def run_command(cmd):
    t = pipes.Template()
    t.append(cmd, "--")
    f = t.open("temp.pipefile", "w")
    f.close()
    data = open('temp.pipefile').read()
    # os.remove('temp.pipefile')
    return data

def parse_data(data):
    skip = True
    headers = list()
    parsed_data = list()
    d_lines = data.split("\n")
    for line in d_lines:
        if "ITER" in line:
            skip = False
            # headers of output
            headers = [h.replace(" ", "") for h in line.split("|")[:-1]]
            parsed_data.append(headers)
            continue
        if skip:
            continue
        i_data = [i.replace(" ", "") for i in line.split("|")[:-1]]
        if len(i_data) == 0:
            continue
        i_data[0] = i_data[0].replace("I:", "")
        parsed_data.append(i_data)
    return parsed_data

def save_json(result_folder, data, filename):
    # check_dirs
    result_folder = result_folder + "/json/"
    if not os.path.exists(result_folder):
        os.makedirs(result_folder)
    # format and save
    with open(result_folder + filename + ".json", "w+") as outfile:
        json.dump(parsed_data, outfile)

def save_xlsx(result_folder, data, filename):
    # check_dirs
    result_folder = result_folder + "/xlsx/"
    if not os.path.exists(result_folder):
        os.makedirs(result_folder)
    # format and save
    from openpyxl import Workbook
    wb = Workbook()
    ws = wb.active
    for i, idata in enumerate(data):
        for j, jdata in enumerate(idata):
            ws[f'{chr(j+65)}{i+1}'] = jdata
    wb.save(result_folder + filename + ".xlsx")

if __name__ == "__main__":
    # pre-defs
    n = 25
    iterCount = 100
    m = 128
    result_folder = "./temp_result_data"

    # # stage1
    # stage = 1
    # for processorCount in [1, 2, 4, 6, 8, 10, 12, 14, 16]:
    #     t0 = time.time()
    #     command = f"./KnapsackTree -n {n} -i {iterCount} -p {processorCount} -m {m}"
    #     print(f"Executing: {command}")
    #     data = run_command(command)
    #     print(data)
    #     t1 = time.time()
    #     # pasre program output
    #     parsed_data = parse_data(data)
    #     print(parsed_data)
    #     filename = f"result-{stage}-{processorCount}"
    #     # saving data to json format
    #     save_json(result_folder, parsed_data, filename)
    #     # saving data to excel format
    #     save_xlsx(result_folder, parsed_data, filename)
    #     # print elspsed time
    #     elapsedTime = t1 - t0
    #     print(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!")
    #     log_file.write(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!\n")

    # # stage2
    # stage = 2
    # for processorCount in [1, 2, 4, 6, 8, 10, 12, 14, 16]:
    #     t0 = time.time()
    #     command = f"./KnapsackTree -n {n} -i {iterCount} -p {processorCount} -m {m} -o"
    #     print(f"Executing: {command}")
    #     data = run_command(command)
    #     print(data)
    #     t1 = time.time()
    #     # pasre program output
    #     parsed_data = parse_data(data)
    #     print(parsed_data)
    #     filename = f"result-{stage}-{processorCount}"
    #     # saving data to json format
    #     save_json(result_folder, parsed_data, filename)
    #     # saving data to excel format
    #     save_xlsx(result_folder, parsed_data, filename)
    #     # print elspsed time
    #     elapsedTime = t1 - t0
    #     print(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!")
    #     log_file.write(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!\n")

    # # stage3.1
    stage = 3
    for relWeight in range(10, 110, 10):
        processorCount = 1
        t0 = time.time()
        command = f"./KnapsackTree -n {n} -i {iterCount} -p {processorCount} -m {m} -r {relWeight} -o"
        print(f"Executing: {command}")
        data = run_command(command)
        print(data)
        t1 = time.time()
        # pasre program output
        parsed_data = parse_data(data)
        ##print(parsed_data)
        filename = f"result-{stage}-{relWeight}-1core"
        # saving data to json format
        save_json(result_folder, parsed_data, filename)
        # saving data to excel format
        save_xlsx(result_folder, parsed_data, filename)
        # print elspsed time
        elapsedTime = t1 - t0
        print(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!")
        log_file.write(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!\n")

    # # stage3.2
    # stage = 3
    # for relWeight in range(10, 110, 10):
    #     processorCount = 16
    #     t0 = time.time()
    #     command = f"./KnapsackTree -n {n} -i {iterCount} -p {processorCount} -m {m} -r {relWeight} -o"
    #     print(f"Executing: {command}")
    #     data = run_command(command)
    #     print(data)
    #     t1 = time.time()
    #     # pasre program output
    #     parsed_data = parse_data(data)
    #     ##print(parsed_data)
    #     filename = f"result-{stage}-{relWeight}"
    #     # saving data to json format
    #     save_json(result_folder, parsed_data, filename)
    #     # saving data to excel format
    #     save_xlsx(result_folder, parsed_data, filename)
    #     # print elspsed time
    #     elapsedTime = t1 - t0
    #     print(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!")
    #     log_file.write(f"Stage {stage} Finished! Elapsed: {elapsedTime} s!\n")

log_file.close()
