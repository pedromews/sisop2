import subprocess
import threading
import random
import time
import re

def run_client(client_id, num_transactions):
    print(f"Starting client {client_id}")
    client_process = subprocess.Popen([
        'docker-compose',
        '-f', 'docker/docker-compose.yml',
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
            
            client_process.stdin.write(transaction)
            client_process.stdin.flush()
            transactions_sent += 1

            # Wait for the response from the server
            response = client_process.stdout.readline().strip()
            if not response:
                print(f"Client {client_id} received no response for transaction {transactions_sent}")
                break
            print(f"Client {client_id} transaction {transactions_sent} response: {response}")

            time.sleep(0.1)  # Increased delay between transactions

    except Exception as e:
        print(f"Error in client {client_id}: {str(e)}")

    finally:
        print(f"Client {client_id} sent {transactions_sent} transactions")
        client_process.stdin.close()
        output, errors = client_process.communicate(timeout=10)
        print(f"Client {client_id} output:", output)
        print(f"Client {client_id} errors:", errors)

def stress_test(num_transactions_per_client):
    threads = []
    for client_id in range(1, 4):  # 3 clients
        thread = threading.Thread(target=run_client, args=(client_id, num_transactions_per_client))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

def get_container_logs(container_name):
    logs = subprocess.run([
        'docker-compose',
        '-f', 'docker/docker-compose.yml',
        'logs',
        container_name
    ], capture_output=True, text=True).stdout
    return logs

def parse_logs():
    server_logs = get_container_logs('server')
    client1_logs = get_container_logs('client1')
    client2_logs = get_container_logs('client2')
    client3_logs = get_container_logs('client3')

    dup_count = len(re.findall(r'DUP', server_logs))
    missing_count = len(re.findall(r'MISSING', server_logs))
    no_funds_count = len(re.findall(r'NO FUNDS', server_logs))

    return server_logs, client1_logs, client2_logs, client3_logs, dup_count, missing_count, no_funds_count

if __name__ == "__main__":
    total_transactions = 300
    transactions_per_client = total_transactions // 3

    start_time = time.time()
    stress_test(transactions_per_client)
    end_time = time.time()

    print(f"Stress test completed.")
    print(f"Total transactions: {total_transactions}")
    print(f"Time taken: {end_time - start_time:.2f} seconds")
    print(f"Transactions per second: {total_transactions / (end_time - start_time):.2f}")

    server_logs, client1_logs, client2_logs, client3_logs, dup, missing, no_funds = parse_logs()

    print("\n=== Server Logs ===")
    print(server_logs)

    print("\n=== Client 1 Logs ===")
    print(client1_logs)

    print("\n=== Client 2 Logs ===")
    print(client2_logs)

    print("\n=== Client 3 Logs ===")
    print(client3_logs)

    print("\n=== Summary ===")
    print(f"DUP requests: {dup}")
    print(f"MISSING requests: {missing}")
    print(f"NO FUNDS requests: {no_funds}")
