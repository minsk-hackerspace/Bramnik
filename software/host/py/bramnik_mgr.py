import click
import string
import random
from datetime import datetime, timedelta
from models import *
import logging
logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s %(name)s %(levelname)-8s %(thread)d %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S")

logger = logging.getLogger("bramnik")



@click.group()
def cli():
    pass

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

@cli.group()
def code():
    pass

@code.command()
@click.argument("authorized_by")
@click.argument("ttl", type=click.INT)
@click.argument("comment")
@click.argument("account_id", required=False)
def emit(authorized_by, ttl, comment, account_id):
    users = User.select().where(User.account_id == authorized_by)
    if len(users) == 0:
        logger.error("Emiting code by unknown user")
        return
    code = ''.join([random.choice(string.digits) for _ in range(13)])
    valid_till = datetime.now() + timedelta(minutes=ttl)
    Code.create(user_id=users[0], code=code, valid_till=valid_till, authorized_by_id=authorized_by, comment=comment)
    logger.info("Code for %s: %s", users[0].name, code)

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

