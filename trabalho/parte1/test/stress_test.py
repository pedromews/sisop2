import subprocess
import threading
import random
import time
import re

def run_client(client_id, num_transactions):
    print(f"Starting client {client_id}")
    client_process = subprocess.Popen([
        'docker-compose',
        '-f', 'trabalho/parte1/docker/docker-compose.yml',
        'exec',
        '-T',
        f'client{client_id}',
        './build/client',
        '4000'
    ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    transactions_sent = 0
    try:
        for _ in range(num_transactions):
            dest = f'172.18.0.{random.randint(2, 4)}'  # Randomly choose client IP
            amount = random.randint(1, 100)
            transaction = f"{dest} {amount}\n"
            
            try:
                client_process.stdin.write(transaction)
                client_process.stdin.flush()
                transactions_sent += 1
            except BrokenPipeError:
                print(f"Client {client_id} process closed unexpectedly")
                break

            time.sleep(0.01)

    except Exception as e:
        print(f"Error in client {client_id}: {str(e)}")

    finally:
        print(f"Client {client_id} sent {transactions_sent} transactions")
        try:
            client_process.stdin.close()
        except:
            pass
        try:
            output, errors = client_process.communicate(timeout=10)
            print(f"Client {client_id} output:", output)
            print(f"Client {client_id} errors:", errors)
        except subprocess.TimeoutExpired:
            print(f"Client {client_id} communication timed out")
            client_process.kill()
            output, errors = client_process.communicate()
        except Exception as e:
            print(f"Error communicating with client {client_id}: {str(e)}")

def stress_test(num_transactions_per_client):
    threads = []
    for client_id in range(1, 4):  # 3 clients
        thread = threading.Thread(target=run_client, args=(client_id, num_transactions_per_client))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

def parse_server_logs():
    server_logs = subprocess.run([
        'docker-compose',
        '-f', 'trabalho/parte1/docker/docker-compose.yml',
        'logs',
        'server'
    ], capture_output=True, text=True).stdout

    dup_count = len(re.findall(r'DUP', server_logs))
    missing_count = len(re.findall(r'MISSING', server_logs))
    no_funds_count = len(re.findall(r'NO FUNDS', server_logs))

    return dup_count, missing_count, no_funds_count

if __name__ == "__main__":
    total_transactions = 100
    transactions_per_client = total_transactions // 3

    start_time = time.time()
    stress_test(transactions_per_client)
    end_time = time.time()

    print(f"Stress test completed.")
    print(f"Total transactions: {total_transactions}")
    print(f"Time taken: {end_time - start_time:.2f} seconds")
    print(f"Transactions per second: {total_transactions / (end_time - start_time):.2f}")

    dup, missing, no_funds = parse_server_logs()
    print(f"DUP requests: {dup}")
    print(f"MISSING requests: {missing}")
    print(f"NO FUNDS requests: {no_funds}")
