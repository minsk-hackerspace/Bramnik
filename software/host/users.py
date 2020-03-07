import sqlite3
conn = sqlite3.connect('users.db')
c = conn.cursor()
rows = c.execute('''select * from users
                     ''')
print rows

# We can also close the connection if we are done with it.
# Just be sure any changes have been committed or they will be lost.
conn.close()
