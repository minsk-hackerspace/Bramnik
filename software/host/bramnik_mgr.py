#!/usr/bin/env python3

import click
import string
import random
import json
import logging
from datetime import datetime, timedelta
from pathlib import Path

from models import *

logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(name)s %(levelname)-8s %(thread)d %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S")

logger = logging.getLogger("bramnik")



@click.group(help="use bramnik_mgr COMMAND --help to get more help")
def cli():
    pass

@cli.group()
def user():
    pass

@user.command(help="Sync user list with json file from site")
@click.argument("file_name")
def sync(file_name):
    user_count = 0
    card_count = 0
    cards_removed = 0
    hackers = json.loads(Path(file_name).read_text())
    hackers_cards = [key.replace(":", "")[-8:] for hacker in hackers for key in hacker["nfc_keys"]]

    logger.info("Clean up all unused cards")
    # Clean up all unused cards
    cards_removed += Card.delete().where(Card.card_id.not_in(hackers_cards)).execute()
    logger.info("Clean up transferred cards")
    # Clean up cards passed from one user to another
    for hacker in hackers:
        hacker_keys = [key.replace(":", "")[-8:] for key in hacker["nfc_keys"]]
        cards_removed += Card.delete().where(Card.user_id==hacker["id"] & Card.card_id.not_in(hacker_keys)).execute()
    print("removed", cards_removed, "cards")

    for hacker in hackers:
        paid_until = hacker["paid_until"]
        if not paid_until:
            paid_until = "1990-01-01"

        valid_till = datetime.strptime(paid_until, "%Y-%m-%d") + timedelta(days=14)
        user, created = User.get_or_create(account_id=hacker["id"], defaults={"name": hacker["name"], "valid_till": valid_till, "access_allowed": hacker["access_allowed"]})
        user_count = user_count + (1 if created else 0)

        updated = user.valid_till != valid_till
        updated |= user.access_allowed != hacker["access_allowed"]
        updated |= user.name != hacker["name"]

        # Update paid time
        if updated:
            user.valid_till = valid_till
            user.access_allowed = hacker["access_allowed"]
            user.name = hacker["name"]
            user.save()

        q = Card.select().where(Card.user_id == user.id)
        existing_keys = []
        for key in q:
            existing_keys.append(key.card_id)

        # Sync cards
        for nfc_key in hacker["nfc_keys"]:
            sanitized_key = nfc_key.replace(":", "")[-8:]
            card, created = Card.get_or_create(user_id=user.id, card_id=sanitized_key)
            try:
                existing_keys.remove(sanitized_key)
            except:
                pass

            card_count = card_count + (1 if created else 0)

    print("created", user_count, "users,", card_count, "cards")

@user.command(help="List all users in system")
def list():
    print("user list:")
    users = User.select()
    columns = "{:4} | {:4} | {:22} | {}"
    print(columns.format("id","account_id","name","valid till"))
    for u in users:
        print(columns.format(u.id, u.account_id, u.name, u.valid_till))


@cli.group()
def card():
    pass

@card.command()
@click.argument("account_id")
@click.argument("card_id")
def add(account_id, card_id): 
    users = User.select().where(User.account_id == account_id)
    if len(users) == 0:
        logger.error("Adding card to unknown user")
        return
    Card.create(user_id=users[0], card_id=card_id)
    logger.debug("user: %s, card %s", users[0], card_id)

@card.command(help="List all cards in system")
def list():
    print("card list:")
    cards = Card.select().join(User)
    columns = "{:4} | {:10} | {:22} | {}"
    print(columns.format("id","number","owner","valid till"))
    for c in cards:
        print(columns.format(c.id, c.card_id, c.user_id.name, c.user_id.valid_till))



@cli.group()
def code():
    pass

@code.command()
@click.argument("authorized_by")
@click.argument("ttl", type=click.INT)
@click.argument("comment")
@click.argument("user_id", required=False)
@click.argument("code_len", default=4, type=click.INT, required=False)
def emit(authorized_by, ttl, comment, user_id, code_len):

    Code.delete().where(Code.valid_till < datetime.now()).execute()

    authorized_by_users = User.select().where(User.account_id == authorized_by)
    if len(authorized_by_users) == 0:
        logger.error("Emiting code by unknown user")
        return
    authorized_by_user = authorized_by_users[0]

    user = None
    if user_id:
        users = User.select().where(User.account_id == user_id)
        if len(users) == 0:
            logger.error("Emitting code for unknown user")
            return
        user = users[0]

    code = ''.join([random.choice(string.digits) for _ in range(code_len)])
    valid_till = datetime.now() + timedelta(minutes=ttl)

    Code.create(user_id=user, code=code, valid_till=valid_till, authorized_by_id=authorized_by_user, comment=comment)

    print("code by {}: {} for {}".format(authorized_by_user.account_id, code, user.account_id if user else "(guest)"))

@code.command()
def list(help="List all codes in system"):
    print("code list")
    A = User.alias()
    B = User.alias()
    codes = Code.select(Code, A, B).join(A, JOIN.LEFT_OUTER, on=(Code.user_id==A.id).alias("user")).switch(Code).join(B, JOIN.LEFT_OUTER, on=(Code.authorized_by_id==B.id).alias("authorized_by"))
    columns = "{:4} | {:8} | {:22} | {:22} | {} | {}"
    print(columns.format("id","code","for","authorized by", "valid till", "comment"))
    for c in codes:
        user_name = c.user.name if hasattr(c, "user") else "(guest)"
        print(columns.format(c.id, c.code, user_name, c.authorized_by.name, c.valid_till, c.comment))

# list users
# list cards
# list codes
# generate code (user_id, authorized_by, ttl)
# add card (user_id, card_id)
# remove card (user_id, card_id)
def init_db():
    db.connect()
    db.create_tables([User, Card, Code])

def main():
    init_db()
    cli()
    

if __name__ =='__main__':
    main()

