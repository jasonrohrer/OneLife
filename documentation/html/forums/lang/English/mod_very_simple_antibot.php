<?php
// Language string for mod Very Simple AntiBot Registration

$lang_mod_vsabr = array(
  'Robot title'      => 'Are you here for the right reasons?',
  'Robot question'   => '<br>Please fill in the blank with a <b>single number</b> (example: 103):<br><b>%s</b>',
  'Robot info'       => 'Today\'s forum spammers are humans, not robots, so they can pass any simple test for humanity.<br>If you\'re here for the right reasons, finding the answer to the following question should be worth your time.',
  'Robot test fail'  => 'You answered incorrectly to the anti-spammer question!',
);


// from here:
// https://stackoverflow.com/questions/2112571/
//         converting-a-number-1-2-3-to-a-string-one-two-three-in-php
/**
    Converts an integer to its textual representation.
     @param num the number to convert to a textual representation
      @param depth the number of times this has been recursed
*/
function readNumber( $num, $depth=0 )
{

    $num = (int)$num;

    $retval ="";

    if ($num < 0) // if it's any other negative, just flip it and call again
        return "negative " + readNumber(-$num, 0);

    if ($num > 99) // 100 and above
    {

        if ($num > 999) // 1000 and higher
            $retval .= readNumber($num/1000, $depth+3);


        $num %= 1000;
        // now we just need the last three digits
        if ($num > 99) // as long as the first digit is not zero
            $retval .= readNumber($num/100, 2)." hundred\n";

        $retval .=readNumber($num%100, 1);
        // our last two digits
        }

    else // from 0 to 99
    {

        $mod = floor($num / 10);

        if ($mod == 0) // ones place
        {

            if ($num == 1) $retval.="one";

            else if ($num == 2) $retval.="two";

            else if ($num == 3) $retval.="three";

            else if ($num == 4) $retval.="four";

            else if ($num == 5) $retval.="five";

            else if ($num == 6) $retval.="six";

            else if ($num == 7) $retval.="seven";

            else if ($num == 8) $retval.="eight";

            else if ($num == 9) $retval.="nine";

            }

        else if ($mod == 1) // if there's a one in the ten's place
        {

            if ($num == 10) $retval.="ten";

            else if ($num == 11) $retval.="eleven";

            else if ($num == 12) $retval.="twelve";

            else if ($num == 13) $retval.="thirteen";

            else if ($num == 14) $retval.="fourteen";

            else if ($num == 15) $retval.="fifteen";

            else if ($num == 16) $retval.="sixteen";

            else if ($num == 17) $retval.="seventeen";

            else if ($num == 18) $retval.="eighteen";

            else if ($num == 19) $retval.="nineteen";

            }

        else // if there's a different number in the ten's place
        {

            if ($mod == 2) $retval.="twenty ";

            else if ($mod == 3) $retval.="thirty ";

            else if ($mod == 4) $retval.="forty ";

            else if ($mod == 5) $retval.="fifty ";

            else if ($mod == 6) $retval.="sixty ";

            else if ($mod == 7) $retval.="seventy ";

            else if ($mod == 8) $retval.="eighty ";

            else if ($mod == 9) $retval.="ninety ";

            if (($num % 10) != 0)
            {

                $retval = rtrim($retval);
                //get rid of space at end
                $retval .= "-";

                }

            $retval.=readNumber($num % 10, 0);

            }

        }


    if ($num != 0)
    {

        if ($depth == 3)
            $retval.=" thousand\n";

        else if ($depth == 6)
            $retval.=" million\n";

        if ($depth == 9)
            $retval.=" billion\n";

        }

    return $retval;

    }


// questions changes every hour
// Note that this might catch an innocent person at the hour mark
// because the question will change in the middle of their registration
// but this is okay.
srand( time() / 3600 );

$mod_vsabr_questions = array();

for( $i=0; $i<50; $i++ ) {

    $numA = rand( 1, 1000 );
    $numB = rand( 1, 1000 );

    $sum = $numA + $numB;

    $numA = readNumber( $numA );
    $numB = readNumber( $numB );
    
    
    $mod_vsabr_questions[ 
        "What do you get when you add <font color=red>$numA</font> to the number <font color=red>$numB</font>?" ] = "$sum";
    }

// return rand back to time-seeded state
list($usec, $sec) = explode(' ', microtime());

srand( $sec + $usec * 1000000 );





/*
// [modif oto] [Very Simple AntiBot questions]
$mod_vsabr_questions = array(
    'What do you call a black and white bird that lives in a cold place? (starts with P)'     => 'Penguin',
    'What do you call a white, fluffy thing that floats in the sky? (starts with C)'     => 'Cloud',
    'What do you call thin white stuff that you write on? (starts with P)'     => 'Paper',
    'What do you call a clear, tasteless liquid that you drink to survive? (starts with W)'     => 'Water',
    'What do you call a very dry, hot place with sand and no water? (starts with D)'     => 'Desert',
    'What do you call water that falls out of the sky? (starts with R)'     => 'Rain',
    'What do you call the big white thing that shines in the sky at night? (starts with M)'     => 'Moon',
    'What do you call a black, furry animal with a white stripe that is very smelly? (starts with S)'     => 'Skunk',
    'What do you call the biggest, thickest finger on your hand? (starts with T)'     => 'Thumb',
    'What do you call the thing on your face that you use to smell? (starts with N)'     => 'Nose',
    'What do you call the hole in your face where the food goes in? (starts with M)'     => 'Mouth',
    'What do you call the fuzzy stuff on the top of your head? (starts with H)'     => 'Hair',
    'What do you call a very very young person? (starts with B)'     => 'Baby',
    'What do you call a white liquid that comes from a female cow? (starts with M)'     => 'Milk',
    'What do you call white, cold, fluffy stuff that falls from the sky? (starts with S)'     => 'Snow',
    'What do you call the bright, warm thing that shines in the sky during the day? (starts with S)'     => 'Sun',
  'What do you call a huge gray animal with a long trunk?  (Starts with E)'     => 'Elephant'
);
*/
?>