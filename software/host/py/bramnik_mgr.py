import click
import string
import random
from datetime import datetime, timedelta
from models import *
import logging
logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(name)s %(levelname)-8s %(thread)d %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S")

logger = logging.getLogger("bramnik")



@click.group(help="use bramnik_mgr COMMAND --help to get more help")
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
def emit(authorized_by, ttl, comment, user_id):
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

    code = ''.join([random.choice(string.digits) for _ in range(6)])
    valid_till = datetime.now() + timedelta(minutes=ttl)

    Code.create(user_id=user, code=code, valid_till=valid_till, authorized_by_id=authorized_by_user, comment=comment)

    logger.error("Created code by %s: %s for %s", authorized_by_user.name, code, user.name if user else "(guest)")

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

