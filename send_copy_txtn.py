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

# Run the function
if __name__ == "__main__":
    send_eth()
