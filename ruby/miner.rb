##
# Implementation of an Primecoin miner in pure ruby
#
# Note this is only for understanding, the performace will be extreamly bad!
#
module XPMiner

  ##
  # Primecoin difficulty
  #
  class Difficulty

    ##
    # bit length of the fractional length part of the difficulty
    #
    # (difficulty = chain_length.fractional_length)
    # (  32 bit   =      8 bit  +  24 bit         )
    #
    N_FRACTIONAL_BITS = 24

    ##
    # bitmasks for the fractional and chain part of the difficulty
    #
    TARGET_FRACTIONAL_MASK = (1 << N_FRACTIONAL_BITS) - 1
    TARGET_LENGTH_MASK     = ~TARGET_FRACTIONAL_MASK

    ##
    # create a new difficulty
    #
    def initialize difficulty = 0
      @difficulty = difficulty
    end

    ##
    # getter method for difficulty
    #
    attr_reader :difficulty

    ##
    # increase a given difficulty by one
    #
    def inc
      @difficulty += TARGET_FRACTIONAL_MASK + 1
    end

    ##
    # adds the given fractional length to a given difficulty
    #
    def add_fractional_length fractional_length
      @difficulty = (@difficulty & TARGET_LENGTH_MASK) | fractional_length
    end

    ##
    # returns the chain length of the given difficulty
    #
    def chain_length 
      return @difficulty >> N_FRACTIONAL_BITS
    end

    ##
    # return @difficulty + n
    #
    def +(n)

      if n.is_a? Fixnum
        return Difficulty.new(@difficulty + (TARGET_FRACTIONAL_MASK + 1) * n)
      elsif n.is_a? Difficulty
        return Difficulty.new(@difficulty + n.difficulty)
      else
       raise ArgumentError.new
      end
    end
  end
  
  ##
  # Class for al the prime seraching an testing stuff
  #
  class Prime
    
    ##
    # Prime Chain types
    #
    PRIME_CHAIN_CUNNINGHAM1 = 1
    PRIME_CHAIN_CUNNINGHAM2 = 2
    PRIME_CHAIN_BI_TWIN     = 3

    ##
    # Calculates the fractional length of a given prime, and its modulo
    # restult form the fermat test
    #
    def get_fractional_length p

      # result of the fermat test
      fermat_res = (1 << (p - 1)) % p

      # res = p - (2^(p - 1) mod p) 
      res = p - fermat_res

      res = res << Difficulty::N_FRACTIONAL_BITS
      fractional_length = res / p

      if fractional_length >= (1 << Difficulty::N_FRACTIONAL_BITS)
         puts "[EE] FermatProbablePrimalityTest() : fractional assert"
      end

      return fractional_length
    end
    
    
    ##
    # Fermat primality test
    #
    # p is likely to be a prime if 2^(p - 1) mod p == 1
    #
    def fermat_test p

      # tmp = 2^(p - 1)
      tmp = 1 << (p - 1)

      return (tmp % p) == 1
    end

    ##
    # Test probable primality of n = 2p +- 1 based on 
    # Euler, Lagrange and Lifchitz
    #
    # sophie_germain:
    #    true:  test for Cunningham Chain of the first kind:  n = 2p + 1
    #    false: test for Cunningham Chain of the second kind: n = 2p - 1
    #
    # returns wether n is a prime or not
    #
    def euler_lagrange_lifchitz_test n, sophie_germain
      
      p = (n - 1) / 2

      # 2^p mod n
      r = (1 << p) % n

      n_mod8      = n % 8
      passed_test = false

      if sophie_germain == true and n_mod8 == 7 
        
        return r == 1
      elsif sophie_germain == true and n_mod8 == 3
        
        return r + 1 == n
      elsif sophie_germain == false and n_mod8 == 5

        return r + 1 == n
      elsif sophie_germain == false and n_mod8 == 1
        
        return r == 1
      else
        puts "[EE] euler_lagrange_lifchitz_test : invalid n % 8 = " +
             n_mod8 + (sophie_germain ? "first kind" : "second kind")
        return false
      end

    end

    ##
    # Test Probable Cunningham Chain for: p
    #
    # sophie_germain:
    #   true:   Test for Cunningham Chain of first kind  (p, 2p+1, 4p+3, ...)
    #   false:  Test for Cunningham Chain of second kind (p, 2p-1, 4p-3, ...)
    #
    # Return value: the difficulty for the tested number
    #/
    def cunningham_chain_test p, sophie_germain, fermat_test, 

      difficulty = Difficulty.new

      # Fermat test for p first 
      if fermat_test(p) == false
        return difficulty
      end

      # Euler-Lagrange-Lifchitz test for the following numbers in chain
      n = p
      
      # loop untill chain end is reatched 
      loop do
      
        difficulty.inc

        # calculate nex numer in the chain
        n = n * 2 + (sophie_germain ? 1 : -1) 

        if fermat_test == true

          break if !fermat_test(n) == false
        else

          break if euler_lagrange_lifchitz_test(n, sophie_germain) == false
        end
      end

      difficulty.add_fractional_length(get_fractional_length(p))

      return difficulty
    end

    ##
    # Test probable prime chain for: origin (prime origin = prime +/- 1)
    #
    # Return value: length of the longest found chain
    #
    def probable_prime_chain_test origin, chain_type
 
      difficulty = Difficulty.new
 
      # Test for Cunningham Chain of first kind
      if chain_type == PRIME_CHAIN_CUNNINGHAM1
      
        difficulty = cunningham_chain_test(origin - 1, true, false)
 
      # Test for Cunningham Chain of second kind
      elsif chain_type == PRIME_CHAIN_CUNNINGHAM2
          
        difficulty = cunningham_chain_test(origin + 1, false, false)
 
      # Test for BiTwin Chains 
      else 
      
        difficulty_1cc = Difficulty.new
        difficulty_2cc = Difficulty.new
 
        difficulty_1cc = cunningham_chain_test(origin - 1, true, false)
 
        if difficulty_1cc.chain_length >= 2
 
          difficulty_2cc = cunningham_chain_test(origin + 1, false, false)
          
          ##
          # Figure out BiTwin Chain length
          # BiTwin Chain allows a single prime at the end for odd length chain
          #
          if difficulty_1cc.chain_length > difficulty_2cc.chain_length
            difficulty = difficulty_2cc + (difficulty_1cc.chain_length + 1)
          else
            difficulty = difficulty_1cc + difficulty_2cc.chain_length
          end
        end
      end
 
      return difficulty
    end
 
    ##
    # test 
    #
    def initialize start = 6
      i = (start / 6) * 6
      
      loop do
        difficulty_1cc = probable_prime_chain_test(i, PRIME_CHAIN_CUNNINGHAM1)
        difficulty_2cc = probable_prime_chain_test(i, PRIME_CHAIN_CUNNINGHAM2)
        difficulty_twn = probable_prime_chain_test(i, PRIME_CHAIN_BI_TWIN)

        len_1cc = difficulty_1cc.chain_length
        len_2cc = difficulty_2cc.chain_length
        len_twn = difficulty_twn.chain_length

        
        puts "[1CC] len: #{ len_1cc } origin: #{ i }" if len_1cc > 2
        puts "[2CC] len: #{ len_2cc } origin: #{ i }" if len_2cc > 2
        puts "[TWN] len: #{ len_twn } origin: #{ i }" if len_twn > 2

        i += 6
      end
    end

  end

end

##
# Test functionality
#
if __FILE__ == $0
  XPMiner::Prime.new 1000000
end

