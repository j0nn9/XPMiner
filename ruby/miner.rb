##
# Implementation of an Primecoin miner in pure ruby
#
# Note this is only for understanding, the performace will be extreamly bad!
#
module XPMiner
  
  ##
  # Class for al the prime seraching an testing stuff
  #
  class Prime
    
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
    # Test probable primality of n = 2p +/- 1 based on 
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

      if sophie_germain == true and n_mod8 == 7 then
        
        return r == 1
      elsif sophie_germain == true and n_mod8 == 3 then
        
        return r + 1 == n
      elsif sophie_germain == false and n_mod8 == 5 then 

        return r + 1 == n
      elsif sophie_germain == false and n_mod8 == 1 then
        
        return r == 1
      else
        puts "[EE] euler_lagrange_lifchitz_test : invalid n % 8 = " +
             n_mod8 + (sophie_germain ? "first kind" : "second kind")
        return false
      end
    end

  end


end


