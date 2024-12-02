from web3 import Web3

import secrets
from secrets import public_address, private_key

web3 = Web3(Web3.HTTPProvider("https://eth-mainnet.g.alchemy.com/v2/_gz0X2XSVyLOpBs1E2PRUOwmbG4fb7aB")) # global Web3 connection
'''
The following 8 arguments are required to create a transaction:
    nonce: transaction count of sender; calculated on the fly
    maxFeePerGas: max wei sender is willing to pay per unit of gas
    maxPriorityFeePerGas: additional amount of wei to incentivize priority
    gas: maximum amount of gas willing to use for the transaction
    to: recipient's address of the transaction; default UNISWAP_ADDRESS
    value: amount of wei being transferred; default value of 0.0001 ETH
    chainId: the Ethereum network where transaction will be executed; default 1
'''
def execute_transaction(maxFeePerGas, maxPriorityFeePerGas, gas):
    DEFAULT_FROM_ADDRESS = web3.to_checksum_address(secrets.public_address) # address we transact from
    DEFAULT_TO_ADDRESS = "0x3fC91A3afd70395Cd496C647d5a6CC9D4B2b7FAD" # uniswap address
    DEFAULT_TO_ADDRESS = web3.to_checksum_address(DEFAULT_TO_ADDRESS) # ensures the address is checksum compliant
    DEFAULT_VALUE = web3.to_wei(0.0001, 'ether') # transaction of 0.0001 ETH per copy
    DEFAULT_CHAINID = 1 # On Ethereum mainnet

    PRIORITY_DELTA = web3.to_wei(0.001, 'gwei') # additional GWEI to get ahead of copied transaction

    nonce = web3.eth.get_transaction_count(DEFAULT_FROM_ADDRESS) # transaction count of sender; used to prevent non-duplicate orders & sabotage attacks

    transaction = {
        'nonce': nonce,
        'maxFeePerGas': maxFeePerGas,
        'maxPriorityFeePerGas': maxPriorityFeePerGas + PRIORITY_DELTA,
        'gas': gas,
        'to': DEFAULT_TO_ADDRESS,
        'value': DEFAULT_VALUE,
        'chainId': DEFAULT_CHAINID
    }

    signed_transaction = web3.eth.account.sign_transaction(transaction, secrets.private_key) # sign transactions using private key
    transaction_result = web3.eth.send_raw_transaction(signed_transaction.raw_transaction) # send transaction to be fulfilled
    print(f'Front run complete with hash:\t {transaction_result}')


#execute_transaction(web3.to_wei(57.152344532, 'gwei'), web3.to_wei(2, 'gwei'), 219758) # test run


