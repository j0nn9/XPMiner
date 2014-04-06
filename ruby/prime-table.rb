##
# implementation of the default Sieve of Eratosthenes to generat 
# a prime table including all primes till a given number starting by 2
#
class PrimeTable

  def initialize sieve_size

    @primes = []
    
    sieve = Array.new sieve_size, true

    
    # 0 and 1 ar not primes 
    sieve[0] = false;
    sieve[0] = false;
 
    # run the sieve 
    for i in (2..Math.sqrt(sieve_size)) do 

      if sieve[i] == true
        
        # p = i^2
        p = i**2

        while p < sieve_size do
          sieve[p] = false
          p += i
        end
      end
    end

    # save the primes
    for i in (2..sieve_size) do 
      
      if sieve[i] == true
        @primes << i
      end
    end

    # no longer needed
    sieve.clear
  end

  ##
  # access the prime at index i
  #
  def [](i)
    return @prime[i]
  end

  ##
  # return the total number of primes
  #
  def num_primes
    return @primes.length
  end

end
