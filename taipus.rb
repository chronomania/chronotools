#
# Finnish name conjugation design tester (1-5 character version)
# for Chrono Trigger Finnish translation
#
# Homepage of the project: http://bisqwit.iki.fi/ctfin/
#
# Conjugates names.
# Doesn't know of any exceptions. (Laki?)
# Doesn't handle plurals. (Names don't usually come in plurals.)
# But handles pretty well everything else.
#
# Also acts as a HOWTO to Finnish language for newbies knowing ruby ;)
#
# Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
#
# Last updated: 29.6.2003


# Define some extensions to String class
class String

  # Is the word front type or back type?
  def front?
    # Find the last character falling to these categories:
    self.gsub(/.*([aouAOUäöyÄÖY0-9])/) do
      return $1 =~ /[äöyÄÖY14579]/
    end
    
    # No hint, take some reasonable default if e/i present
    return true if self =~ /[eiéEIÉ]/
    
    # No vowels. Guess from the last character.
    return self !~ /[hkqåHKQÅ]/
  end
  
  # Does the word have a vowel somewhere and not number after it?
  def hasvowel?
    return self =~ /[aeiouyäöåéAEIOUYÄÖÅÉ][^0-9]*$/
  end
  
  # Get the nth last character of the word
  def lastchar(ind)
    return self.slice(0,1) if size < ind
    self.slice(-ind, 1)
  end
  
  # Is this a vowel? (For 1 char strings)
  def isvowel?
    self =~ /[aeiouyåäöéAEIOUYÅÄÖÉ]/
  end
  
  # Is this k/p/t (hard consonant)? (For 1 char strings)
  def ishard?
    self =~ /[kptKPT]/
  end
  
  # Is this word probably a nonword?
  def isabbrev?
    size == 1 or not hasvowel?
  end
  
  # Ends with a vowel?
  def endwithvowel?
    lastchar(1).isvowel?
  end
  
  # Does the word end with es/us/ys?
  def esloppu?
    self =~ /[aeiouyåäöéAEIOUYÅÄÖÉ][sS]$/
  end
  
  # Does the word end with hard*2 + vowel?
  def doublehard?
    size >= 3 \
    and endwithvowel? \
    and lastchar(2) == lastchar(3) \
    and lastchar(2).ishard?
  end
  
  # Does the word end with -aki/-äki, e desired?
  def akiend_e?
    self =~ /[aäAÄ][kptKPT][iI]$/
  end

  # Does the word end with -aki/-äki, i desired?
  def akiend_i?
    # FIXME: laki -> lain, not laen (or??)
    false
  end
end

# Hard stem (used in partitive)
#
def hardstem(s, plural=false)
  ret = String.new(s)
  if s.isabbrev?
    # Abbreviation. This isn't very good but there's no better.
    # Examples: X,a -> X:ää
    #           A,a -> A:ta
    #           P,a -> P:tä
    ret << ':'
    last = s.lastchar(1)
    if last =~ /.*[flmnrswx479]/i
      ret << 'ä'
    elsif last =~ /[zZ]/
      ret << 'a'
    elsif last !~ /[0-9]/
      ret << 't'
    end
  elsif s.esloppu?
    # If the word ends with s, add "t"
    # Example: Magus,a -> Magusta
    if plural
      ret[-1, 1] = 'ksi'
    else
      ret << 't'
    end
  elsif not s.endwithvowel?
    # If the word does not end with vowel, add "i"
    # Examples: John,a -> Johnia
    #           Frog,a -> Frogia
    
    ret << (plural ? 'ej' : 'i')
  elsif s.akiend_e?
    # If the word ends with aki or äki, hard ending is ake/äke
    ret[-1, 1] = 'e'
  elsif s.doublehard?
    # If the word ends with hardconsonantdouble + vowel,
    ret[-1, 1] = 'ej'  if(plural)
  end
  ret
end

# Soft stem (used in almost everything)
#
def softstem(s, plural=false)
  ret = String.new(s)
  if s.isabbrev?
    # Abbreviation. This isn't very good but there's no better.
    # Example: X,lle -> X:lle
    ret << ':'
  elsif s.esloppu?
    # If the word ends with vowel+s, replace s with "kse"
    # Example: Magus,lle -> Magukselle
    ret[-1, 1] = plural ? 'ksi' : 'kse'
  elsif not s.endwithvowel?
    # If the word does not end with vowel, add "i"
    # Example: John,a -> Johnia
    ret << (plural ? 'i' : 'i')
  elsif s.doublehard?
    # If the word ends with hardconsonantdouble + vowel,
    # Remove one consonant (second last char)
    # Example: Mikko,lle -> Mikolle

    ret[-2,1] = ''
    ret << 'i' if(plural and ret.lastchar(1) != 'i')
  elsif s.akiend_e?
    # If the word ends with aki or äki, hard ending is ae/äe
    ret = ret.slice(0..-3) + 'e'
    ret << 'i' if(plural)
  elsif s.akiend_i?
    # If the word ends with aki or äki, hard ending is ai/äi
    ret = ret.slice(0..-3) + 'i'
  end
  ret
end

# basic form
#               example: crono
def nominatiiviksi(s, plural=false)
  (plural ? softstem(s) + 't' : s)
end

# partitiivi  = the object of action that affects the target
#               example: cronoa
def partitiiviksi(s, plural=false)
  hardstem(s, plural) + (s.front? ? 'ä' : 'a')
end

# akkusatiivi = the object of action that affects the target as whole
#               example: cronon
def akkusatiiviksi(s, plural=false)
  softstem(s, plural) + (plural ? 't' : 'n')
end

# genetiivi   = owner of something, in English usually "'s"
#               example: cronon
def genetiiviksi(s, plural=false)
  softstem(s, plural) + 'n'
end

# adessiivi   = owner/container of something, in English "has" or "in" or "on"
#               example: cronolla
def adessiiviksi(s, plural=false)
  softstem(s, plural) + (s.front? ? 'llä' : 'lla')
end

# elatiivi   = source of something
#              example: cronosta
def elatiiviksi(s, plural=false)
  softstem(s, plural) + (s.front? ? 'stä' : 'sta')
end

# allatiivi   = target of something, in English sometimes as "to" or "for" preposition
#               example: cronolle
def allatiiviksi(s, plural=false)
  softstem(s, plural) + 'lle'
end


def testit(nimi, plural=false)
  puts "--" + nominatiiviksi(nimi, plural) + "--"
  puts "Pasi näki " + akkusatiiviksi(nimi, plural) + " (Pasi saw " + nimi + ")"
  puts "Pasi kiittää " + partitiiviksi(nimi, plural) + " (Pasi thanks " + nimi + ")"
  puts genetiiviksi(nimi, plural) + " kello on rikki (" + nimi + "'s clock is broken)"
  puts adessiiviksi(nimi, plural) + " on kissa (" + nimi + " has a cat)"
  puts elatiiviksi(nimi, plural) + " se kissa on kaunis (" + nimi + " thinks the cat is cute)"
  puts allatiiviksi(nimi, plural) + " soitettiin monta kertaa (" + nimi + " was called many times)"
end


testit "ZGX"
testit "R66"
testit "Magus"
testit "Teräs"
testit "John"

testit "Matti"
testit "ARLA"
testit "Kaisa"
testit "Lätsä"
testit "Mäki"

testit "Pelle"
