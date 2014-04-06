require 'digest'

##
# impelmentaion of the primecoin block header
#
# (new transactions are saved into a block,
#  but for poolmining we only need the block header)
#
class BlockHeader
  
  ##
  # current version taken form 
  # the original client
  #
  CURRENT_VERSION = 2

  ##
  # creates an new BlockHeader
  #
  def initialize
    @version           = CURRENT_VERSION
    @hash_prev_block   = 0
    @hash_merkle_root  = 0
    @time              = 0
    @min_difficulty    = 0
    @nonce             = 0
    @primemultiplier   = 0
  end

  # for testing TODO
  attr_accessor :version, :hash_prev_block, :hash_merkle_root, :time, :min_difficulty 
  attr_accessor :nonce, :primemultiplier

  ##
  # returns the binary string repersentation of this
  #
  def to_s
    str  = [ @version ].pack("L") 
    str += sha256_to_str(@hash_prev_block)
    str += sha256_to_str(@hash_merkle_root)
    str += [ @time, @min_difficulty, @nonce ].pack("LLL")
  end

  ##
  # calculates and returns the header hash
  #
  # (int converts the values to its binary representation befor hashing)
  #
  def hash
    tmp = Digest::SHA256.new
    tmp.update self.to_s
    
    # hash the result again
    sha256 = Digest::SHA256.new
    sha256.update(tmp.digest)

    return sha256.digest
  end

  private
  
  ##
  # helper function to convert a sha256 bignum to a binary string
  #
  def sha256_to_str b
    bytes = [ ]

    32.times do
      bytes << (b & 0xFF)
      b >>= 8
    end

    return bytes.pack("C*")
  end
end

##
# testing
#

# TODO include in inintialize (reading from string)
def bytestr_to_i str
  int = 0;

  str.unpack("C*").reverse.each do |e|
    int |= e
    int <<= 8
  end
  
  int >>= 8

  return int
end

##
# hash a given header
#
if __FILE__ == $0

  if ARGV.length != 1
    puts "#{ $0 } <file>"
    exit
  end

  file = File.open(ARGV[0], "rb")
  
  ##
  # comanline is <version> <hash_prev_block> <hash_merkle_root> <time> <min_difficulty> <nonce>
  #
  header = BlockHeader.new
  header.version          = file.sysread(4).unpack("L")[0]
  header.hash_prev_block  = bytestr_to_i(file.sysread(32)) 
  header.hash_merkle_root = bytestr_to_i(file.sysread(32)) 
  header.time             = file.sysread(4).unpack("L")[0]
  header.min_difficulty   = file.sysread(4).unpack("L")[0]
  header.nonce            = file.sysread(4).unpack("L")[0]

  puts "Header:"
  puts "  version #{ header.version }"
  puts "  hash_prev_block #{ header.hash_prev_block }"
  puts "  hash_merkle_root #{ header.hash_merkle_root }"
  puts "  time #{ header.time }"
  puts "  min_difficulty #{ header.min_difficulty }"
  puts "  nonce #{ header.nonce }"

  print "\nheader hash: "

  header.hash.each_byte { |e| printf "%02x", e }
  print "\n"

end
