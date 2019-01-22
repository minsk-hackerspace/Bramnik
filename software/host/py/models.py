from peewee import *

db = SqliteDatabase('access.db')

class BaseModel(Model):
  class Meta:
    database = db

class User(BaseModel):
  class Meta:
    db_table = 'users'
  account_id = IntegerField(unique=True)
  valid_till = DateTimeField()
  name = TextField()

class Code(BaseModel):
  class Meta:
    db_table = 'codes'
  user_id = ForeignKeyField(User, backref='codes')
  authorized_by = ForeignKeyField(User, backref='given_codes')
  valid_till = DateTimeField()
  comment = TextField()
  code = TextField()

class Card(BaseModel):
  class Meta:
    db_table = 'cards'
  user_id = ForeignKeyField(User, backref='cards')
  card_id = TextField(unique=True)

