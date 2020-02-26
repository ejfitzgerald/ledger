
from fetchai.ledger.crypto import Entity
from fetchai.ledger.crypto import Address
from fetchai.ledger.api import LedgerApi

with open('p2p.key', 'rb') as key:
    entity = Entity(key.read())
    address = Address(entity)
    ledger_api = LedgerApi('127.0.0.1', 8000)
    print(ledger_api.tokens.balance(entity))
