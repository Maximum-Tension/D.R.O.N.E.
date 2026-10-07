#!/usr/bin/env python3
"""Builds Lucy's knowledge packages (.mem files) and the package catalog.

A package line names a thing (and other names for it), says what it is, and
lists what is known about it:

    cat, kitty: A cat is a small furry animal. | kind: animal, mammal, pet | legs: 4

Lucy reads every .mem file in mind/memory when she starts, and any new,
changed or removed one before each message, so a package can be added while
she runs.  catalog.txt lists every package that exists (installed or not)
with the words it covers; when she is asked about a word she doesn't know,
she names the package to add.

    python3 tools/make_packages.py            # writes mind/memory and packages
    python3 tools/make_packages.py OUTDIR     # writes OUTDIR/mind/memory, OUTDIR/packages

Edit the tables below (or the .mem files themselves) to teach Lucy more.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# ---------------------------------------------------------------- things --
# name(s): {property: value}.  "kind" may hold several kinds.
THINGS = {}


def thing(names, **props):
    key = names
    entry = THINGS.setdefault(key, {})
    for k, v in props.items():
        if v is None:
            continue
        if k == 'kind' and k in entry:
            kinds = [x.strip() for x in entry[k].split(',')]
            for extra in str(v).split(','):
                if extra.strip() not in kinds:
                    kinds.append(extra.strip())
            entry[k] = ', '.join(kinds)
            continue
        entry[k] = str(v)


# animals: legs, kind, young, sound, can, cannot, eats, lives
A = 'animal'
thing('cat, kitty', kind='animal, mammal, pet', legs=4, young='kitten',
      sound='meow', can='climb, jump', eats='fish, mice', female='queen',
      male='tom')
thing('dog, doggy', kind='animal, mammal, pet', legs=4, young='puppy',
      sound='bark', can='swim, run', eats='meat')
thing('cow', kind='animal, mammal, farm animal', legs=4, young='calf',
      sound='moo', eats='grass', male='bull', female='cow', gives='milk')
thing('bull', kind='animal, mammal, farm animal', legs=4, young='calf')
thing('horse', kind='animal, mammal, farm animal', legs=4, young='foal',
      sound='neigh', eats='grass, hay', male='stallion', female='mare')
thing('pig', kind='animal, mammal, farm animal', legs=4, young='piglet',
      sound='oink', male='boar', female='sow')
thing('sheep', kind='animal, mammal, farm animal', legs=4, young='lamb',
      sound='baa', eats='grass', male='ram', female='ewe', gives='wool')
thing('goat', kind='animal, mammal, farm animal', legs=4, young='kid',
      sound='bleat', male='billy goat', female='nanny goat')
thing('chicken, hen', kind='animal, bird, farm animal', legs=2, wings=2,
      young='chick', sound='cluck', gives='eggs', male='rooster', female='hen')
thing('rooster', kind='animal, bird, farm animal', legs=2, wings=2,
      sound='cock-a-doodle-doo')
thing('duck', kind='animal, bird', legs=2, wings=2, young='duckling',
      sound='quack', can='swim, fly', male='drake')
thing('goose', kind='animal, bird', legs=2, wings=2, young='gosling',
      sound='honk', can='swim, fly', male='gander')
thing('owl', kind='animal, bird', legs=2, wings=2, young='owlet', sound='hoot',
      can='fly')
thing('bird', kind='animal', legs=2, wings=2, young='chick', sound='tweet',
      can='fly')
thing('eagle', kind='animal, bird', legs=2, wings=2, young='eaglet',
      can='fly')
thing('swan', kind='animal, bird', legs=2, wings=2, young='cygnet',
      can='swim, fly')
thing('parrot', kind='animal, bird, pet', legs=2, wings=2, young='chick',
      sound='squawk', can='fly, talk')
thing('penguin', kind='animal, bird', legs=2, wings=2, young='chick',
      can='swim', cannot='fly', lives='Antarctica')
thing('ostrich', kind='animal, bird', legs=2, wings=2, young='chick',
      can='run', cannot='fly')
thing('lion', kind='animal, mammal, wild animal', legs=4, young='cub',
      sound='roar', eats='meat', female='lioness', lives='Africa')
thing('tiger', kind='animal, mammal, wild animal', legs=4, young='cub',
      sound='roar', eats='meat', female='tigress')
thing('bear', kind='animal, mammal, wild animal', legs=4, young='cub',
      sound='growl')
thing('wolf', kind='animal, mammal, wild animal', legs=4, young='pup',
      sound='howl', eats='meat')
thing('fox', kind='animal, mammal, wild animal', legs=4, young='cub')
thing('elephant', kind='animal, mammal, wild animal', legs=4, young='calf',
      sound='trumpet', eats='plants')
thing('giraffe', kind='animal, mammal, wild animal', legs=4, young='calf')
thing('zebra', kind='animal, mammal, wild animal', legs=4, young='foal',
      color='black and white')
thing('camel', kind='animal, mammal', legs=4, young='calf',
      lives='the desert')
thing('donkey', kind='animal, mammal, farm animal', legs=4, young='foal',
      sound='bray')
thing('deer', kind='animal, mammal, wild animal', legs=4, young='fawn')
thing('rabbit, bunny', kind='animal, mammal, pet', legs=4, young='kit',
      eats='carrots, grass')
thing('mouse', kind='animal, mammal', legs=4, young='pup', sound='squeak',
      eats='cheese, seeds')
thing('rat', kind='animal, mammal', legs=4, young='pup', sound='squeak')
thing('hamster', kind='animal, mammal, pet', legs=4, young='pup')
thing('squirrel', kind='animal, mammal', legs=4, young='kit', eats='nuts')
thing('monkey', kind='animal, mammal, primate', young='infant',
      can='climb')
thing('kangaroo', kind='animal, mammal, marsupial', legs=2, young='joey',
      can='jump', lives='Australia')
thing('koala', kind='animal, mammal, marsupial', legs=4, young='joey',
      lives='Australia', eats='eucalyptus leaves')
thing('bat', kind='animal, mammal', legs=2, wings=2, young='pup', can='fly')
thing('whale', kind='animal, mammal', legs=0, young='calf', can='swim',
      lives='the sea')
thing('dolphin', kind='animal, mammal', legs=0, young='calf', can='swim',
      lives='the sea', sound='click')
thing('shark', kind='animal, fish', legs=0, young='pup', can='swim',
      lives='the sea')
thing('fish', kind='animal', legs=0, young='fry', can='swim',
      lives='water')
thing('goldfish', kind='animal, fish, pet', legs=0, can='swim',
      color='orange')
thing('octopus', kind='animal, mollusk', legs=8, arms=8, can='swim',
      lives='the sea')
thing('crab', kind='animal, crustacean', legs=10, can='swim',
      lives='the sea')
thing('snail', kind='animal, mollusk', legs=0, young='hatchling')
thing('snake', kind='animal, reptile', legs=0, young='hatchling',
      sound='hiss')
thing('lizard', kind='animal, reptile', legs=4, young='hatchling')
thing('turtle', kind='animal, reptile', legs=4, young='hatchling',
      can='swim')
thing('crocodile', kind='animal, reptile', legs=4, young='hatchling',
      can='swim')
thing('frog', kind='animal, amphibian', legs=4, young='tadpole',
      sound='croak', can='jump, swim')
thing('spider', kind='animal, arachnid', legs=8, young='spiderling',
      eats='insects')
thing('ant', kind='animal, insect', legs=6, young='larva')
thing('bee', kind='animal, insect', legs=6, wings=4, young='larva',
      sound='buzz', can='fly', gives='honey')
thing('butterfly', kind='animal, insect', legs=6, wings=4,
      young='caterpillar', can='fly')
thing('fly', kind='animal, insect', legs=6, wings=2, young='maggot',
      sound='buzz', can='fly')
thing('mosquito', kind='animal, insect', legs=6, wings=2, can='fly')
thing('beetle', kind='animal, insect', legs=6)
thing('ladybug, ladybird', kind='animal, insect', legs=6, can='fly',
      color='red with black spots')
thing('insect', kind='animal', legs=6)
thing('human, person', kind='mammal', legs=2, arms=2, fingers=10, toes=10,
      young='baby', eyes=2, ears=2)

# people and family
thing('man', kind='person', young='boy', opposite='woman')
thing('woman', kind='person', young='girl', opposite='man')
thing('boy', kind='person', opposite='girl')
thing('girl', kind='person', opposite='boy')
thing('king', kind='person', opposite='queen')
thing('queen', kind='person', opposite='king')
thing('prince', kind='person', opposite='princess')
thing('princess', kind='person', opposite='prince')
thing('father, dad', kind='parent, person', opposite='mother')
thing('mother, mom, mum', kind='parent, person', opposite='father')
thing('brother', kind='person', opposite='sister')
thing('sister', kind='person', opposite='brother')
thing('son', kind='child, person', opposite='daughter')
thing('daughter', kind='child, person', opposite='son')
thing('uncle', kind='person', opposite='aunt')
thing('aunt', kind='person', opposite='uncle')
thing('husband', kind='person', opposite='wife')
thing('wife', kind='person', opposite='husband')
thing('grandfather, grandpa', kind='person', opposite='grandmother')
thing('grandmother, grandma', kind='person', opposite='grandfather')

# body
thing('hand', kind='body part', fingers=5)
thing('foot', kind='body part', toes=5)
thing('head', kind='body part', eyes=2, ears=2, nose=1, mouth=1)
for part in ['arm', 'leg', 'eye', 'ear', 'nose', 'mouth', 'finger', 'toe',
             'knee', 'elbow', 'shoulder', 'neck', 'back', 'tooth', 'tongue',
             'heart', 'brain', 'stomach', 'lung', 'skin', 'hair']:
    thing(part, kind='body part')

# vehicles and things with wheels
thing('car', kind='vehicle', wheels=4, uses='petrol or electricity')
thing('bicycle, bike', kind='vehicle', wheels=2, pedals=2)
thing('tricycle', kind='vehicle', wheels=3)
thing('unicycle', kind='vehicle', wheels=1)
thing('motorcycle, motorbike', kind='vehicle', wheels=2)
thing('bus', kind='vehicle')
thing('truck, lorry', kind='vehicle')
thing('train', kind='vehicle')
thing('plane, airplane, aeroplane', kind='vehicle', wings=2, can='fly')
thing('helicopter', kind='vehicle', can='fly')
thing('boat, ship', kind='vehicle', can='float')
thing('skateboard', kind='toy', wheels=4)
thing('scooter', kind='vehicle', wheels=2)

# furniture and objects
thing('table', kind='furniture', legs=4)
thing('chair', kind='furniture', legs=4)
thing('stool', kind='furniture', legs=3)
thing('bed', kind='furniture')
thing('sofa, couch', kind='furniture')
thing('desk', kind='furniture')
thing('clock', kind='device', hands=3)
thing('watch', kind='device', hands=3)
thing('phone', kind='device')
thing('computer', kind='device, machine')

# time
thing('minute', kind='unit of time', seconds=60)
thing('hour', kind='unit of time', minutes=60, seconds=3600)
thing('day', kind='unit of time', hours=24, minutes=1440, seconds=86400,
      opposite='night')
thing('week', kind='unit of time', days=7, hours=168)
thing('fortnight', kind='unit of time', days=14, weeks=2)
thing('month', kind='unit of time', weeks='about 4', days='28 to 31')
thing('year', kind='unit of time', months=12, weeks=52, days=365, seasons=4)
thing('leap year', kind='unit of time', months=12, days=366)
thing('decade', kind='unit of time', years=10)
thing('century', kind='unit of time', years=100)
thing('millennium', kind='unit of time', years=1000)
thing('dozen', kind='number', things=12)
thing('pair', kind='number', things=2)
thing('score', kind='number', things=20)

DAYS = ['monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday',
        'sunday']
for index, name in enumerate(DAYS):
    thing(name, kind='day of the week, day',
          next=DAYS[(index + 1) % 7].capitalize(),
          before=DAYS[index - 1].capitalize())
MONTHS = [('january', 31), ('february', '28 (29 in a leap year)'),
          ('march', 31), ('april', 30), ('may', 31), ('june', 30),
          ('july', 31), ('august', 31), ('september', 30), ('october', 31),
          ('november', 30), ('december', 31)]
for index, (name, days) in enumerate(MONTHS):
    thing(name, kind='month', days=days, order=index + 1,
          next=MONTHS[(index + 1) % 12][0].capitalize(),
          before=MONTHS[index - 1][0].capitalize())
for name, after in [('spring', 'summer'), ('summer', 'autumn'),
                    ('autumn, fall', 'winter'), ('winter', 'spring')]:
    thing(name, kind='season', next=after)
thing('summer', opposite='winter')
thing('winter', opposite='summer')

# shapes
for name, sides in [('triangle', 3), ('square', 4), ('rectangle', 4),
                    ('pentagon', 5), ('hexagon', 6), ('heptagon', 7),
                    ('octagon', 8), ('nonagon', 9), ('decagon', 10)]:
    thing(name, kind='shape', sides=sides, corners=sides, angles=sides)
thing('circle', kind='shape', sides=0, corners=0)
thing('oval', kind='shape', sides=0, corners=0)
thing('cube', kind='shape', faces=6, edges=12, corners=8)
thing('pyramid', kind='shape', faces=5, edges=8, corners=5)
thing('sphere, ball', kind='shape', faces=1, edges=0, corners=0)

# colors
for name in ['red', 'orange', 'yellow', 'green', 'blue', 'purple', 'pink',
             'brown', 'black', 'white', 'gray, grey', 'violet', 'indigo',
             'gold', 'silver', 'turquoise', 'beige']:
    thing(name, kind='color')
thing('black', opposite='white')
thing('white', opposite='black')
thing('rainbow', colors=7)

# food
FRUITS = [('apple', 'red or green'), ('banana', 'yellow'),
          ('orange', 'orange'), ('grape', 'purple or green'),
          ('strawberry', 'red'), ('lemon', 'yellow'), ('lime', 'green'),
          ('cherry', 'red'), ('pear', 'green'), ('watermelon', 'green'),
          ('pineapple', 'yellow'), ('mango', 'orange'), ('peach', 'orange'),
          ('plum', 'purple'), ('kiwi', 'brown'), ('blueberry', 'blue'),
          ('raspberry', 'red'), ('coconut', 'brown'), ('melon', 'green'),
          ('tomato', 'red'), ('avocado', 'green'), ('fig', 'purple'),
          ('apricot', 'orange'), ('pomegranate', 'red')]
for name, color in FRUITS:
    thing(name, kind='fruit, food', color=color)
VEGETABLES = [('carrot', 'orange'), ('potato', 'brown'),
              ('broccoli', 'green'), ('lettuce', 'green'), ('onion', 'white'),
              ('cucumber', 'green'), ('pea', 'green'), ('corn', 'yellow'),
              ('spinach', 'green'), ('cabbage', 'green'),
              ('pumpkin', 'orange'), ('pepper', 'red, green or yellow'),
              ('garlic', 'white'), ('celery', 'green'), ('bean', 'green'),
              ('cauliflower', 'white'), ('eggplant, aubergine', 'purple'),
              ('radish', 'red'), ('beet, beetroot', 'red'),
              ('zucchini, courgette', 'green'), ('mushroom', 'white')]
for name, color in VEGETABLES:
    thing(name, kind='vegetable, food', color=color)
thing('tomato', kind='fruit, food',
      note='a fruit, though it is often cooked like a vegetable')
for name in ['bread', 'cheese', 'rice', 'pasta', 'pizza', 'soup', 'egg',
             'meat', 'chicken', 'cake', 'cookie', 'sandwich', 'chocolate',
             'butter', 'honey', 'salad']:
    thing(name, kind='food')
thing('chicken, hen', kind='animal, bird, farm animal, food')
thing('milk', kind='drink', color='white')
thing('water', kind='drink', color='clear')
for name in ['juice', 'tea', 'coffee', 'lemonade', 'soda', 'cocoa']:
    thing(name, kind='drink')
thing('coffee', color='brown')
thing('snow', color='white', kind='weather')
thing('grass', color='green', kind='plant')
thing('sky', color='blue')
thing('sun', color='yellow', kind='star')
thing('blood', color='red')
thing('coal', color='black')
thing('cloud', color='white or gray', kind='weather')
for name in ['rain', 'wind', 'fog', 'storm', 'hail', 'thunder', 'lightning']:
    thing(name, kind='weather')

# what things are like, so "is fire hot?" and "are lemons sour?" have answers
TRAITS = [('fire', 'hot, bright, dangerous'), ('sun', 'hot, bright'),
          ('ice', 'cold, hard, slippery'), ('snow', 'cold, soft'),
          ('water', 'wet'), ('rain', 'wet'), ('sugar', 'sweet'),
          ('honey', 'sweet, sticky'), ('candy', 'sweet'),
          ('chocolate', 'sweet'), ('lemon', 'sour'), ('lime', 'sour'),
          ('salt', 'salty'), ('stone, rock', 'hard, heavy'),
          ('feather', 'light, soft'), ('pillow', 'soft'), ('knife', 'sharp'),
          ('night', 'dark'), ('glass', 'hard, clear'), ('cotton', 'soft'),
          ('iron', 'hard, heavy'), ('elephant', 'big, heavy'),
          ('mouse', 'small'), ('ant', 'small'), ('whale', 'big'),
          ('giraffe', 'tall'), ('snail', 'slow'), ('turtle', 'slow'),
          ('cheetah', 'fast'), ('rabbit', 'fast'), ('desert', 'hot, dry'),
          ('ocean, sea', 'big, deep, salty'), ('oven', 'hot'),
          ('fridge', 'cold'), ('pepper', 'spicy'), ('coffee', 'hot'),
          ('mountain', 'tall, big'), ('baby', 'small'), ('blood', 'red')]
for names, traits in TRAITS:
    thing(names, traits=traits)

# clothing, tools, instruments, sports, places, jobs
for name in ['shirt', 'pants, trousers', 'dress', 'skirt', 'shoe', 'sock',
             'hat', 'coat, jacket', 'scarf', 'glove', 'sweater, jumper',
             'boot', 'belt', 'tie']:
    thing(name, kind='clothing')
for name in ['hammer', 'saw', 'screwdriver', 'wrench, spanner', 'drill',
             'shovel', 'axe', 'pliers', 'scissors', 'knife']:
    thing(name, kind='tool')
for name in ['piano', 'guitar', 'violin', 'drum', 'flute', 'trumpet',
             'saxophone', 'harp', 'cello', 'clarinet']:
    thing(name, kind='musical instrument, instrument')
thing('guitar', strings=6)
thing('violin', strings=4)
thing('piano', keys=88)
for name in ['football, soccer', 'basketball', 'tennis', 'swimming',
             'running', 'baseball', 'volleyball', 'golf', 'hockey', 'boxing',
             'cycling', 'skiing']:
    thing(name, kind='sport')
thing('football, soccer', players=11)
thing('basketball', players=5)
thing('volleyball', players=6)
for name in ['teacher', 'doctor', 'nurse', 'farmer', 'pilot', 'cook, chef',
             'driver', 'police officer', 'firefighter', 'baker', 'dentist',
             'engineer', 'programmer', 'artist', 'singer', 'writer']:
    thing(name, kind='job, person')
for name in ['house', 'school', 'hospital', 'shop, store', 'park', 'farm',
             'beach', 'library', 'bank', 'restaurant', 'zoo', 'museum',
             'airport', 'station']:
    thing(name, kind='place, building')
for name in ['mountain', 'river', 'lake', 'sea, ocean', 'forest', 'desert',
             'island', 'hill', 'valley']:
    thing(name, kind='place, nature')

# space
thing('sun', kind='star', planets=8)
thing('moon', kind='moon', color='gray')
thing('earth', kind='planet', moons=1, order=3)
thing('solar system', planets=8)
for name, order, moons in [('mercury', 1, 0), ('venus', 2, 0),
                           ('mars', 4, 2), ('jupiter', 5, 95),
                           ('saturn', 6, 146), ('uranus', 7, 28),
                           ('neptune', 8, 16)]:
    thing(name, kind='planet', order=order, moons=moons)

# numbers and letters
thing('alphabet', letters=26, vowels=5)
thing('english alphabet', letters=26, vowels=5)
for name in ['a', 'e', 'i', 'o', 'u']:
    pass

# ---------------------------------------------------------------- words --
OPPOSITES = [
    ('hot', 'cold'), ('up', 'down'), ('big', 'small'), ('large', 'small'),
    ('tall', 'short'), ('long', 'short'), ('fast', 'slow'),
    ('quick', 'slow'), ('happy', 'sad'), ('good', 'bad'), ('light', 'dark'),
    ('heavy', 'light'), ('day', 'night'), ('open', 'closed'),
    ('in', 'out'), ('on', 'off'), ('yes', 'no'), ('left', 'right'),
    ('right', 'wrong'), ('old', 'new'), ('young', 'old'), ('rich', 'poor'),
    ('strong', 'weak'), ('hard', 'soft'), ('easy', 'hard'),
    ('difficult', 'easy'), ('wet', 'dry'), ('full', 'empty'),
    ('loud', 'quiet'), ('noisy', 'quiet'), ('early', 'late'),
    ('first', 'last'), ('high', 'low'), ('near', 'far'),
    ('inside', 'outside'), ('above', 'below'), ('over', 'under'),
    ('before', 'after'), ('push', 'pull'), ('give', 'take'),
    ('buy', 'sell'), ('win', 'lose'), ('start', 'finish'),
    ('begin', 'end'), ('love', 'hate'), ('friend', 'enemy'),
    ('laugh', 'cry'), ('true', 'false'), ('clean', 'dirty'),
    ('thick', 'thin'), ('fat', 'thin'), ('wide', 'narrow'),
    ('deep', 'shallow'), ('sweet', 'sour'), ('safe', 'dangerous'),
    ('alive', 'dead'), ('awake', 'asleep'), ('brave', 'cowardly'),
    ('north', 'south'), ('east', 'west'), ('top', 'bottom'),
    ('front', 'back'), ('more', 'less'), ('many', 'few'),
    ('always', 'never'), ('everything', 'nothing'),
    ('everyone', 'no one'), ('come', 'go'), ('arrive', 'leave'),
    ('remember', 'forget'), ('question', 'answer'), ('ask', 'answer'),
    ('add', 'subtract'), ('plus', 'minus'), ('positive', 'negative'),
    ('odd', 'even'), ('male', 'female'), ('sunrise', 'sunset'),
    ('cheap', 'expensive'), ('polite', 'rude'), ('kind', 'cruel'),
    ('beautiful', 'ugly'), ('smooth', 'rough'), ('sharp', 'blunt'),
    ('tight', 'loose'), ('lost', 'found'), ('borrow', 'lend'),
    ('import', 'export'), ('maximum', 'minimum'),
    ('increase', 'decrease'), ('accept', 'refuse'),
    ('agree', 'disagree'), ('entrance', 'exit'), ('send', 'receive'),
    ('teach', 'learn'), ('success', 'failure'), ('pass', 'fail'),
    ('war', 'peace'), ('true', 'false'), ('even', 'odd'),
    ('morning', 'evening'), ('wake', 'sleep'), ('sit', 'stand'),
    ('stop', 'go'), ('throw', 'catch'), ('forward', 'backward'),
    ('found', 'lost'), ('hello', 'goodbye'), ('hi', 'bye'),
    ('together', 'apart'), ('top', 'bottom'), ('outside', 'inside'),
    ('upstairs', 'downstairs'), ('asleep', 'awake'), ('calm', 'angry'),
    ('busy', 'free'), ('wild', 'tame'), ('raw', 'cooked'),
    ('fresh', 'stale'), ('public', 'private'), ('real', 'fake'),
    ('same', 'different'), ('simple', 'complicated'), ('cool', 'warm'),
    ('warm', 'cool'), ('freeze', 'melt'), ('rise', 'fall'),
    ('float', 'sink'), ('found', 'lost'), ('tidy', 'messy'),
    ('interesting', 'boring'), ('strong', 'weak'), ('love', 'hate'),
    ('hungry', 'full'), ('heaven', 'hell'), ('city', 'countryside'),
    ('land', 'sea'), ('soft', 'loud'), ('whisper', 'shout'),
    ('crooked', 'straight'), ('curly', 'straight'), ('bright', 'dim'),
    ('quiet', 'loud'), ('little', 'big'), ('tiny', 'huge'),
]

# word meanings from the old basics pack go to words.mem when their subject
# is a describing word, the rest to things_object.mem
WORD_SUBJECTS = set()
for first, second in OPPOSITES:
    WORD_SUBJECTS.add(first)
    WORD_SUBJECTS.add(second)


def read_sentences(path):
    """(names, sentence) for each sentence of an old knowledge pack."""
    out = []
    if not os.path.exists(path):
        return out
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        parts = re.split(r'(?<=[.!?])\s+(?=[A-Z])', line)
        for part in parts:
            names = subject_names(part)
            out.append((names, part))
    return out


def subject_names(sentence):
    """"A plane or airplane is ..." -> [plane, airplane]."""
    text = sentence.strip()
    match = re.match(r'^(?:To\s+)?(?:(?:A|An|The)\s+)?([A-Za-z][A-Za-z \-\']*?)'
                     r'\s+(?:is|are|means|has|have|was)\b', text)
    if not match:
        return []
    subject = match.group(1).strip().lower()
    names = []
    for piece in re.split(r'\s+or\s+|,\s*|\s+and\s+', subject):
        piece = piece.strip()
        for article in ('a ', 'an ', 'the '):
            if piece.startswith(article):
                piece = piece[len(article):]
        if piece and len(piece.split()) <= 3:
            names.append(piece)
    return names


def singular(word):
    if word.endswith('ies') and len(word) > 4:
        return word[:-3] + 'y'
    if word.endswith('s') and not word.endswith('ss') and len(word) > 3:
        return word[:-1]
    return word


def entry_line(names, sentence, props):
    pieces = [sentence] if sentence else []
    for key in sorted(props):
        pieces.append('%s: %s' % (key, props[key]))
    return '%s: %s' % (names, ' | '.join(pieces))


def build_things_and_words(old_basics):
    """things_object.mem and words.mem lines."""
    definitions = {}
    order = []
    for names, sentence in read_sentences(old_basics):
        if not names:
            names = ['general']
        key = names[0]
        if key not in definitions:
            definitions[key] = {'names': names, 'sentences': []}
            order.append(key)
        definitions[key]['sentences'].append(sentence)

    thing_lines = []
    word_lines = []
    used = set()

    # the tables, with a sentence when the old pack had one
    for names, props in THINGS.items():
        name_list = [n.strip() for n in names.split(',')]
        sentence = ''
        for candidate in name_list + [singular(n) for n in name_list]:
            if candidate in definitions:
                sentence = ' '.join(definitions[candidate]['sentences'])
                used.add(candidate)
                break
        if not sentence and 'kind' in props:
            kinds = [k.strip() for k in props['kind'].split(',')]
            # the most telling kind: "farm animal" over "mammal" over "animal"
            telling = [k for k in kinds if k not in ('animal', 'pet', 'food',
                                                     'person', 'place')]
            first_kind = telling[-1] if telling else kinds[0]
            # names of their own: "Mercury is a planet", "Monday is a day"
            if kinds[0] in ('planet', 'month', 'day of the week', 'season'):
                sentence = '%s is %s %s.' % (
                    name_list[0].capitalize(),
                    'an' if kinds[0][0] in 'aeiou' else 'a', kinds[0])
                first_kind = ''
            article = 'an' if name_list[0][0] in 'aeiou' else 'a'
            kind_article = 'an' if first_kind[:1] in ('a', 'e', 'i', 'o',
                                                      'u') else 'a'
            if first_kind and first_kind in ('color', 'shape', 'drink', 'food', 'sport',
                              'weather', 'clothing', 'tool', 'month',
                              'season', 'planet', 'fruit', 'vegetable',
                              'furniture', 'vehicle', 'animal', 'mammal',
                              'bird', 'insect', 'reptile', 'fish', 'job',
                              'place', 'body part', 'device', 'person',
                              'parent', 'child', 'unit of time', 'number',
                              'musical instrument', 'day of the week',
                              'star', 'plant', 'machine', 'toy', 'moon',
                              'farm animal', 'wild animal', 'arachnid',
                              'amphibian', 'mollusk', 'crustacean',
                              'marsupial', 'primate', 'building', 'nature',
                              'instrument', 'unit of time'):
                if first_kind in ('color',):
                    sentence = '%s is a color.' % name_list[0].capitalize()
                elif first_kind in ('sport', 'weather', 'food', 'drink',
                                    'clothing') and name_list[0] in (
                        'football', 'soccer', 'basketball', 'tennis',
                        'swimming', 'running', 'baseball', 'volleyball',
                        'golf', 'hockey', 'boxing', 'cycling', 'skiing',
                        'rain', 'wind', 'fog', 'hail', 'thunder',
                        'lightning', 'bread', 'cheese', 'rice', 'pasta',
                        'soup', 'meat', 'butter', 'honey', 'chocolate',
                        'juice', 'tea', 'coffee', 'lemonade', 'soda',
                        'cocoa', 'milk', 'water', 'pants', 'trousers'):
                    sentence = '%s is %s %s.' % (
                        name_list[0].capitalize(),
                        'a' if first_kind != 'weather' else 'a kind of',
                        first_kind)
                    sentence = sentence.replace('a kind of weather',
                                                'a kind of weather')
                else:
                    sentence = '%s %s is %s %s.' % (
                        article.capitalize(), name_list[0], kind_article,
                        first_kind)
        line = entry_line(', '.join(name_list), sentence, props)
        thing_lines.append(line)
        used.update(name_list)

    # opposites, one line per word
    opposite_of = {}
    for first, second in OPPOSITES:
        opposite_of.setdefault(first, [])
        if second not in opposite_of[first]:
            opposite_of[first].append(second)
        opposite_of.setdefault(second, [])
        if first not in opposite_of[second]:
            opposite_of[second].append(first)

    for word in sorted(opposite_of):
        if word in [n.strip() for names in THINGS for n in names.split(',')]:
            continue
        sentence = ''
        if word in definitions:
            sentence = ' '.join(definitions[word]['sentences'])
            used.add(word)
        word_lines.append(entry_line(word, sentence,
                                     {'opposite': ', '.join(opposite_of[word]),
                                      'kind': 'word'}))

    # the old pack's other sentences
    for key in order:
        if key in used:
            continue
        info = definitions[key]
        sentence = ' '.join(info['sentences'])
        line = entry_line(', '.join(info['names']), sentence, {})
        if key in WORD_SUBJECTS or key.endswith('ing') or \
                sentence.split(' ')[1:2] == ['means'] or \
                ' means ' in sentence[:40]:
            word_lines.append(line)
        else:
            thing_lines.append(line)
        used.add(key)

    # the things that have an opposite get it in their own line
    for index, line in enumerate(thing_lines):
        names = line.split(':')[0]
        first = names.split(',')[0].strip()
        if first in opposite_of and 'opposite:' not in line:
            thing_lines[index] = line + ' | opposite: ' + \
                ', '.join(opposite_of[first])
    return thing_lines, word_lines


def build_computers(old_computers):
    lines = []
    seen = {}
    order = []
    for names, sentence in read_sentences(old_computers):
        if not names:
            names = ['computer use']
        key = names[0]
        if key not in seen:
            seen[key] = {'names': names, 'sentences': []}
            order.append(key)
        seen[key]['sentences'].append(sentence)
    for key in order:
        info = seen[key]
        lines.append(entry_line(', '.join(info['names']),
                                ' '.join(info['sentences']),
                                {'kind': 'computer word'}))
    return lines


# ---------------------------------------------------------------- places --
COUNTRIES = [
    # country, capital, continent, language
    ('France', 'Paris', 'Europe', 'French'),
    ('Germany', 'Berlin', 'Europe', 'German'),
    ('Poland', 'Warsaw', 'Europe', 'Polish'),
    ('Italy', 'Rome', 'Europe', 'Italian'),
    ('Spain', 'Madrid', 'Europe', 'Spanish'),
    ('Portugal', 'Lisbon', 'Europe', 'Portuguese'),
    ('United Kingdom, UK, Britain', 'London', 'Europe', 'English'),
    ('England', 'London', 'Europe', 'English'),
    ('Ireland', 'Dublin', 'Europe', 'English and Irish'),
    ('Netherlands, Holland', 'Amsterdam', 'Europe', 'Dutch'),
    ('Belgium', 'Brussels', 'Europe', 'Dutch, French and German'),
    ('Switzerland', 'Bern', 'Europe', 'German, French, Italian and Romansh'),
    ('Austria', 'Vienna', 'Europe', 'German'),
    ('Sweden', 'Stockholm', 'Europe', 'Swedish'),
    ('Norway', 'Oslo', 'Europe', 'Norwegian'),
    ('Denmark', 'Copenhagen', 'Europe', 'Danish'),
    ('Finland', 'Helsinki', 'Europe', 'Finnish and Swedish'),
    ('Greece', 'Athens', 'Europe', 'Greek'),
    ('Turkey', 'Ankara', 'Europe and Asia', 'Turkish'),
    ('Russia', 'Moscow', 'Europe and Asia', 'Russian'),
    ('Ukraine', 'Kyiv', 'Europe', 'Ukrainian'),
    ('Czech Republic, Czechia', 'Prague', 'Europe', 'Czech'),
    ('Hungary', 'Budapest', 'Europe', 'Hungarian'),
    ('Romania', 'Bucharest', 'Europe', 'Romanian'),
    ('United States, USA, America, US', 'Washington, D.C.', 'North America',
     'English'),
    ('Canada', 'Ottawa', 'North America', 'English and French'),
    ('Mexico', 'Mexico City', 'North America', 'Spanish'),
    ('Brazil', 'Brasilia', 'South America', 'Portuguese'),
    ('Argentina', 'Buenos Aires', 'South America', 'Spanish'),
    ('Chile', 'Santiago', 'South America', 'Spanish'),
    ('Peru', 'Lima', 'South America', 'Spanish'),
    ('Colombia', 'Bogota', 'South America', 'Spanish'),
    ('China', 'Beijing', 'Asia', 'Chinese'),
    ('Japan', 'Tokyo', 'Asia', 'Japanese'),
    ('South Korea, Korea', 'Seoul', 'Asia', 'Korean'),
    ('India', 'New Delhi', 'Asia', 'Hindi and English'),
    ('Pakistan', 'Islamabad', 'Asia', 'Urdu and English'),
    ('Indonesia', 'Jakarta', 'Asia', 'Indonesian'),
    ('Thailand', 'Bangkok', 'Asia', 'Thai'),
    ('Vietnam', 'Hanoi', 'Asia', 'Vietnamese'),
    ('Philippines', 'Manila', 'Asia', 'Filipino and English'),
    ('Iran', 'Tehran', 'Asia', 'Persian'),
    ('Iraq', 'Baghdad', 'Asia', 'Arabic and Kurdish'),
    ('Saudi Arabia', 'Riyadh', 'Asia', 'Arabic'),
    ('Israel', 'Jerusalem', 'Asia', 'Hebrew'),
    ('Egypt', 'Cairo', 'Africa', 'Arabic'),
    ('Nigeria', 'Abuja', 'Africa', 'English'),
    ('Kenya', 'Nairobi', 'Africa', 'Swahili and English'),
    ('South Africa', 'Pretoria', 'Africa', 'many languages'),
    ('Morocco', 'Rabat', 'Africa', 'Arabic and Berber'),
    ('Ethiopia', 'Addis Ababa', 'Africa', 'Amharic'),
    ('Australia', 'Canberra', 'Oceania', 'English'),
    ('New Zealand', 'Wellington', 'Oceania', 'English and Maori'),
]
CONTINENTS = ['Africa', 'Antarctica', 'Asia', 'Australia', 'Europe',
              'North America', 'South America']
OCEANS = ['Pacific Ocean', 'Atlantic Ocean', 'Indian Ocean',
          'Southern Ocean', 'Arctic Ocean']


def build_places():
    lines = []
    for names, capital, continent, language in COUNTRIES:
        first = names.split(',')[0].strip()
        sentence = '%s is a country in %s. Its capital is %s.' % (
            first, continent, capital)
        lines.append(entry_line(names.lower(), sentence, {
            'kind': 'country, place', 'capital': capital,
            'continent': continent, 'language': language}))
    capitals = {}
    for names, capital, continent, language in COUNTRIES:
        first = names.split(',')[0].strip()
        capitals.setdefault(capital, first)
    for capital, country in capitals.items():
        lines.append(entry_line(capital.lower(),
                                '%s is the capital of %s.' % (capital,
                                                              country),
                                {'kind': 'city, capital, place',
                                 'country': country}))
    for continent in CONTINENTS:
        lines.append(entry_line(continent.lower(),
                                '%s is one of the seven continents.' %
                                continent,
                                {'kind': 'continent, place'}))
    lines.append(entry_line('world, earth', '', {'continents': 7,
                                                 'oceans': 5}))
    for ocean in OCEANS:
        lines.append(entry_line(ocean.lower(),
                                'The %s is one of the five oceans.' % ocean,
                                {'kind': 'ocean, place'}))
    return lines


def header(name, about):
    return ['# %s.mem: a knowledge package for Lucy.' % name,
            '# about: %s' % about,
            '# Each line: name, other names: what it is | property: value '
            '| property: value',
            '# Put this file in the mind/memory folder and Lucy knows it at '
            'once, no restart needed.', '']


def write(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as stream:
        stream.write('\n'.join(lines) + '\n')


def names_of(lines):
    out = []
    for line in lines:
        if line.startswith('#') or ':' not in line:
            continue
        for name in line.split(':')[0].split(','):
            name = name.strip()
            if name and name not in out:
                out.append(name)
    return out


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else ROOT
    old = os.path.join(HERE, 'old_knowledge')
    things, words = build_things_and_words(os.path.join(old, 'basics.txt'))
    computers = build_computers(os.path.join(old, 'computers.txt'))
    places = build_places()

    packages = [
        ('things_object', 'everyday animals, people, things, food, shapes, '
         'colors and time, and how many legs, wheels and sides they have',
         things, os.path.join(out, 'mind', 'memory')),
        ('words', 'what describing words mean and their opposites', words,
         os.path.join(out, 'mind', 'memory')),
        ('computers', 'computers, files and how Lucy uses her actions',
         computers, os.path.join(out, 'mind', 'memory')),
        ('places', 'countries, their capitals, continents and languages, '
         'and the oceans', places, os.path.join(out, 'packages')),
    ]
    catalog = ['# Every knowledge package that exists, and the words it '
               'knows about.',
               '# package: what it is about | word, word, ...', '']
    for name, about, lines, folder in packages:
        write(os.path.join(folder, name + '.mem'), header(name, about) + lines)
        catalog.append('%s.mem: %s | %s' % (name, about,
                                            ', '.join(names_of(lines))))
        print('%s.mem: %d lines -> %s' % (name, len(lines), folder))
    write(os.path.join(out, 'mind', 'memory', 'catalog.txt'), catalog)


if __name__ == '__main__':
    main()
