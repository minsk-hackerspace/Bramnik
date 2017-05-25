require 'active_record'

class User < ActiveRecord::Base
  has_many :nfc_cards

  def self.check_access(data, db_file = 'users.txt')
    users = File.readlines db_file
    users.any? do |user|
      data == user
    end
  end

  def self.check_code(data, db_file = 'codes.txt')
    codes = File.readlines db_file
    codes.any? do |code|
      data == code
    end
  end
end