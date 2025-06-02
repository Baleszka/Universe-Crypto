from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib.parse
import os
import random
import hashlib


ACCOUNTS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "Accounts"))

charset1 = ['a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','1','2','3','4','5','6','7','8','9','0']
charset2 = ['a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','1','2','3','4','5','6','7','8','9','0']
charset3 = ['a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','1','2','3','4','5','6','7','8','9','0']

class RequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        body = self.rfile.read(content_length).decode()
        data = urllib.parse.parse_qs(body)

        if self.path == "/login":
            username = data.get("username", [None])[0]
            password = data.get("password", [None])[0]

            if not username or not password:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Missing username or password")
                return

            filepath = os.path.join(ACCOUNTS_DIR, f"{username}.db")

            if not os.path.exists(filepath):
                self.send_response(404)
                self.end_headers()
                self.wfile.write(b"Account not found")
                return

            with open(filepath, "r") as f:
                lines = f.readlines()
                first_line = lines[0].rstrip("\n")
                stored_password = first_line

            if password == stored_password:
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"Login successful")
            else:
                self.send_response(403)
                self.end_headers()
                self.wfile.write(b"Incorrect password")

        elif self.path == "/create_account":
            username = data.get("username", [None])[0]
            password = data.get("password", [None])[0]

            if not username or not password:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Missing username or password")
                return

            filepath = os.path.join(ACCOUNTS_DIR, f"{username}.db")

            if os.path.exists(filepath):
                self.send_response(409)
                self.end_headers()
                self.wfile.write(b"Account already exists")
                return

            with open(filepath, "w") as f:
                f.write(password)
                f.write(f"\n0.0")

            self.send_response(201)
            self.end_headers()
            self.wfile.write(b"Account created successfully")

        elif self.path == "/check_balance":
            try:
                username = data.get("username", [None])[0]
                filepath = os.path.join(ACCOUNTS_DIR, f"{username}.db")
        
                if not os.path.exists(filepath):
                    self.send_response(404)
                    self.end_headers()
                    self.wfile.write(b"Account not found")
                    return

                with open(filepath, "r") as f:
                    lines = f.readlines()

                balance = lines[1].rstrip("\n")

                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"Balance: ")
                self.wfile.write(balance.encode())

            except Exception as e:
                print(f"Exception in /check_balance: {e}")
                self.send_response(500)
                self.end_headers()
                self.wfile.write(b"Internal server error")

        elif self.path == "/send_crypto":
            try:
                sender = data.get("sender", [None])[0]
                reciever = data.get("reciever", [None])[0]
                amount = float(data.get("amount", [0])[0])

                senderpath = os.path.join(ACCOUNTS_DIR, f"{sender}.db")
                recieverpath = os.path.join(ACCOUNTS_DIR, f"{reciever}.db")

                if not os.path.exists(senderpath) or not os.path.exists(recieverpath):
                    self.send_response(404)
                    self.end_headers()
                    self.wfile.write(b"Sender or receiver not found.")
                    return

                with open(senderpath, "r") as f:
                    lines = f.readlines()
                    sender_pass = lines[0].rstrip("\n")
                    sender_balance = float(lines[1].rstrip("\n"))

                if sender_balance < amount:
                    self.send_response(403)
                    self.end_headers()
                    self.wfile.write(b"Insufficient funds.")
                    return

                with open(recieverpath, "r") as r:
                    rec_lines = r.readlines()
                    reciever_pass = rec_lines[0].rstrip("\n")
                    reciever_balance = float(rec_lines[1].rstrip("\n"))

                sender_balance -= amount
                reciever_balance += amount

                with open(senderpath, "w") as f:
                    f.write(f"{sender_pass}\n{sender_balance}")
                with open(recieverpath, "w") as f:
                    f.write(f"{reciever_pass}\n{reciever_balance}")

                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"Transaction successful.")

            except Exception as e:
                print(f"Exception in /send_crypto: {e}")
                self.send_response(500)
                self.end_headers()
                self.wfile.write(b"Internal server error.")

        elif self.path.startswith("/check_hash"):
            try:
                hash_string = data.get("hash_string", [None])[0]
                hash_value = data.get("hash", [None])[0]
                wallet = data.get("wallet", [None])[0]
                
                if not hash_string or not hash_value or not wallet:
                    self.send_response(400)
                    self.end_headers()
                    self.wfile.write(b"Missing required parameters")
                    return

                computed_hash = hashlib.sha256(hash_string.encode()).hexdigest()
                if computed_hash == hash_value:
                    difficulty = int(self.path[-1]) if self.path[-1].isdigit() else 1
                    reward_map = {1: 0.03, 2: 0.2, 3: 2}
                    reward = reward_map.get(difficulty, 0.03)
                    
                    print(f"Mining success: Difficulty={difficulty}, Reward={reward}")
                    
                    filepath = os.path.join(ACCOUNTS_DIR, f"{wallet}.db")
                    if os.path.exists(filepath):
                        with open(filepath, "r") as f:
                            lines = f.readlines()
                        password = lines[0].rstrip("\n")
                        balance = float(lines[1].rstrip("\n")) + reward
                        
                        with open(filepath, "w") as f:
                            f.write(f"{password}\n{balance}")
                        
                        self.send_response(200)
                        self.end_headers()
                        self.wfile.write(f"Mining successful! Earned {reward} coins.".encode())
                    else:
                        self.send_response(404)
                        self.end_headers()
                        self.wfile.write(b"Wallet not found")
                else:
                    self.send_response(400)
                    self.end_headers()
                    self.wfile.write(b"Invalid hash")
            
            except Exception as e:
                print(f"Exception in /check_hash: {e}")
                self.send_response(500)
                self.end_headers()
                self.wfile.write(b"Internal server error")

    def do_GET(self):
        if self.path == "/get_hash_1":
            tohash1 = "".join(random.choice(charset1) for _ in range(4))
            hash1 = hashlib.sha256(tohash1.encode()).hexdigest()

            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(hash1.encode())

        elif self.path == "/get_hash_2":
            tohash2 = "".join(random.choice(charset2) for _ in range(5))
            hash2 = hashlib.sha256(tohash2.encode()).hexdigest()

            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(hash2.encode())

        elif self.path == "/get_hash_3":
            tohash3 = "".join(random.choice(charset3) for _ in range(5))
            hash3 = hashlib.sha256(tohash3.encode()).hexdigest()

            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(hash3.encode())
        elif self.path.startswith("/get_adress"):
            query = urllib.parse.urlparse(self.path).query
            params = urllib.parse.parse_qs(query)
            target_person = params.get("adress", [None])[0]
    
            if not target_person:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Missing adress parameter")
                return

            target_hash = hashlib.sha1(target_person.encode()).hexdigest()
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(target_hash.encode())


        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
        


def run():
    os.makedirs(ACCOUNTS_DIR, exist_ok=True)
    
    server = HTTPServer(("localhost", 8000), RequestHandler)
    print("Server running at http://localhost:8000")
    print("Reward system: Difficulty 1=0.03, Difficulty 2=0.2, Difficulty 3=2")
    server.serve_forever()

if __name__ == "__main__":
    run()