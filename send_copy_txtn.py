from web3 import Web3

def send_eth():
    node_url = "http://localhost:8545"  # Your full node URL
    web3 = Web3(Web3.HTTPProvider(node_url))

    # Replace with your private key and addresses
    private_key = "PRIVATE_KEY"  # Replace with your private key
    from_address = "0x437a6a9c014B1a640981105D0465e6f1BC5b4290"
    to_address = "0x0293cEC882b36491eb065d144e9faA5157E3A114"
    value_in_ether = 0.0001  # ETH to send
    chain_id = 1  # Ethereum Mainnet Chain ID

    # Fetch the nonce for the sender address
    nonce = web3.eth.get_transaction_count(from_address)

    # Build the transaction
    tx = {
        'nonce': nonce,
        'to': to_address,
        'value': web3.to_wei(value_in_ether, 'ether'),
        'gas': 21000,
        'gasPrice': web3.to_wei(20, 'gwei'),
        'chainId': chain_id  # Add the chain ID
    }

    # Sign the transaction
    signed_tx = web3.eth.account.sign_transaction(tx, private_key)

    # Send the signed transaction
    tx_hash = web3.eth.send_raw_transaction(signed_tx.raw_transaction)
    print(f"Transaction sent! Hash: {web3.to_hex(tx_hash)}")



def ethical_front_run():
    # Connect to your Ethereum node (local Geth)
    w3 = Web3(Web3.HTTPProvider("http://localhost:8545"))

    # Check connection
    if not w3.is_connected():
        print("Failed to connect to the Ethereum node.")
        exit()

# Sender address and private key
    # Convert addresses to checksum format
    sender_address = Web3.to_checksum_address("0x437a6a9c014B1a640981105D0465e6f1BC5b4290")
    recipient_address = Web3.to_checksum_address("0x3fc91a3afd70395cd496C647d5a6CC9D4b2B7FAD")

    private_key = "4b488479df9e498499ac23d47f2206453e4bb2d9cec446b5cce61bacde10f2cb"  # Replace with the sender's private key

    balance = w3.eth.get_balance(sender_address)
    print(f"Balance of {sender_address}: {w3.from_wei(balance, 'ether')} ETH")
# Re-create the transaction
    transaction = {
        "nonce": w3.eth.get_transaction_count(sender_address),  # Current nonce
        "to": recipient_address ,  # Recipient address
        "value": w3.to_wei(0.0001, "ether"),  # Value in ETH
        "gas": 219758,  # Gas limit
        "maxFeePerGas": w3.to_wei(23.152344532, "gwei"),  # Max fee per gas
        "maxPriorityFeePerGas": w3.to_wei(2, "gwei"),  # Max priority fee per gas
        "chainId": 1,  # Ethereum mainnet
        "data":  "0x3593564c000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000674cc01800000000000000000000000000000000000000000000000000000000000000040b000604000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000e0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002800000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000005af3107a40000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000005af3107a4000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002bc02aaa39b223fe8d0a0e5c4f27ead9083c756cc20001f42260fac5e5542a773aa44fbcfedf7c193bc2c59900000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000600000000000000000000000002260fac5e5542a773aa44fbcfedf7c193bc2c599000000000000000000000000000000fee13a103a10d593b9ae06b3e05f2e7e1c000000000000000000000000000000000000000000000000000000000000001900000000000000000000000000000000000000000000000000000000000000600000000000000000000000002260fac5e5542a773aa44fbcfedf7c193bc2c599000000000000000000000000437a6a9c014b1a640981105d0465e6f1bc5b429000000000000000000000000000000000000000000000000000000000000001690c" }

    # Sign the transaction
    signed_tx = w3.eth.account.sign_transaction(transaction, private_key)

    # Send the transaction
    tx_hash = w3.eth.send_raw_transaction(signed_tx.raw_transaction)

    print(f"Ethical front-run complete! Hash: {tx_hash.hex()}")



# Run the function
if __name__ == "__main__":
    ethical_front_run()
