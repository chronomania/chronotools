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


# Define some extensions to String class
class String

  # Is the word front type or back type?
  def front?
    # Find the last character falling to these categories:
    last = self.gsub(/.*([aouäöyAOUÄÖY123456789]).*/, '\1')
    if last.size == 1
      # If definitely front?
      return true if last =~ /[äöyÄÖY14579]/
      # If definitely back?
      return false if last =~ /[aou02368]/i
    end
    # Use default. (What default?)
    return true
    #self.slice(0,1) =~ /[hkqz]/i
  end
  
  # Does the word have a vowel somewhere and not number after it?
  def hasvowel?
    return self =~ /[aeiouyäöåéÄÖÅÉ][^0123456789]*$/i
  end
  
  # Get the nth last character of the word
  def lastchar(ind)
    return self.slice(0,1) if size < ind
    self.slice(-ind,1)
  end
  
  # Is this a vowel? (For 1 char strings)
  def isvowel?
    self =~ /[aeiouyåäöéÅÄÖÉ]/i
  end
  
  # Is this k/p/t (hard consonant)? (For 1 char strings)
  def ishard?
    self =~ /[kpt]/i
  end
  
  # Is this word probably a nonword?
  def isabbrev?
    size == 1 or not hasvowel?
  end
  
  def endwithvowel?
    lastchar(1).isvowel?
  end
  
  # Does the word end with es/us?
  def esloppu?
    self =~ /[aeiouåäöéÅÄÖÉ]s$/i
  end
  
  # Does the word end with hard*2 + vowel?
  def doublehard?
    size > 2 \
    and lastchar(1).isvowel? \
    and lastchar(2) == lastchar(3) \
    and lastchar(2).ishard?
  end
  
  # Does the word end with -aki/-äki, e desired?
  def akiend_e?
    self =~ /[aäÄ][kpt]i$/i
  end

  # Does the word end with -aki/-äki, i desired?
  def akiend_i?
    # FIXME: laki -> lain, not laen (or??)
    false
  end
end

def hardbody(s, plural=false)
  ret = String.new(s)
  if s.isabbrev?
    # Abbreviation. This isn't very good but there's no better.
    # Examples: X,a -> X:ää
    #           A,a -> A:ta
    #           P,a -> P:tä
    ret << ':'
    last = s.lastchar(1)
    if last =~ /[flmnlrswx479]/i
      ret << 'ä'
    elsif last =~ /[z]/i 
      ret << 'a'
    elsif last !~ /[0123456789]/i
      ret << 't'
    end
  elsif s.esloppu?
    # If the word ends with s, add "t"
    # Example: Magus,a -> Magusta
    if(plural)
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
    ret[-1,1] = 'ej'  if(plural)
  end
  ret
end


def softbody(s, plural=false)
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
  if plural
    ret = softbody(s, plural)
    ret << 't'
  else
    ret = String.new(s)
  end
end

# partitiivi  = the object of action that affects the target
#               example: cronoa
def partitiiviksi(s, plural=false)
  ret = hardbody(s, plural)
  ret << (ret.front? ? 'ä' : 'a')
end

# akkusatiivi = the object of action that affects the target as whole
#               example: cronon
def akkusatiiviksi(s, plural=false)
  ret = softbody(s, plural)
  ret << (plural ? 't' : 'n')
end

# genetiivi   = owner of something, in English usually "'s"
#               example: cronon
def genetiiviksi(s, plural=false)
  ret = softbody(s, plural)
  ret << 'n'
end

# adessiivi   = owner/container of something, in English "has" or "in" or "on"
#               example: cronolla
def adessiiviksi(s, plural=false)
  ret = softbody(s, plural)
  ret << 'll'
  ret << (ret.front? ? 'ä' : 'a')
end

# allatiivi   = target of something, in English sometimes as "to" or "for" preposition
#               example: cronolle
def allatiiviksi(s, plural=false)
  ret = softbody(s, plural)
  ret << 'lle'
end


def testit(nimi, plural=false)
  puts "--" + nominatiiviksi(nimi, plural) + "--"
  puts "Pasi näki " + akkusatiiviksi(nimi, plural) + " (Pasi saw " + nimi + ")"
  puts "Pasi kiittää " + partitiiviksi(nimi, plural) + " (Pasi thanks " + nimi + ")"
  puts genetiiviksi(nimi, plural) + " kello on rikki (" + nimi + "'s clock is broken)"
  puts adessiiviksi(nimi, plural) + " on kissa (" + nimi + " has a cat)"
  puts allatiiviksi(nimi, plural) + " soitettiin monta kertaa (" + nimi + " was called many times)"
end


testit "ZGK"
testit "ZGX"
testit "R1234"
testit "Magus"
testit "Teräs"
testit "John"

testit "Matti"
testit "Mökki"
testit "Lucca"
testit "Lätsä"
testit "Mäki"

testit "Pelle"
