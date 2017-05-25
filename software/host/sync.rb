require 'active_record'
require 'typhoeus'
require 'base64'
require 'digest'


class Sync
  def initialize
    ActiveRecord::Base.logger = Logger.new(File.open('database.log', 'w'))
    ActiveRecord::Base.establish_connection(adapter: 'sqlite3', database: 'users.sqilte3')
    ActiveRecord::Schema.define do
      unless ActiveRecord::Base.connection.tables.include? 'users'
        create_table :users do |table|
          table.column :name, :string
          table.column :email, :string
          table.column :valid_till, :date
        end

        create_table :nfc_cards do |table|
          table.column :user_id, :integer
          table.column :token, :string
        end
      end
    end
  end

  def generate_token
    token = ''
    64.times { token << rand(255) }
    @token ||= Base64.encode64 token
  end

  def run
    generate_token
    response = Typhoeus.get('https://hackerspace.by/users', body: @token)
    if response.code == 200
      @body = JSON.parse response.body, symbolize_names: true
      update_database if check_response
    end
  end

  def update_database
    @body[:users].each do |user|
      db_user = User.find_or_create(user)
      user[:nfc_cards].each do |card|
        db_user.nfc_cards.find_or_create(card)
      end
    end
  end

  def check_response
    Digest::SHA1.hexdigest(@token + ENV['SECRET_KEY_BASE']) == @body[:hexdigest]
  end
end