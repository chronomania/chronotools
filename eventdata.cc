#include <map>
#include <list>
#include <vector>
#include <cstdio>

#include "eventdata.hh"
#include "miscfun.hh"
#include "romaddr.hh"
#include "base62.hh"
#include "ctcset.hh"

typedef unsigned char Byte;

/* Pointers to these events are starting from 0x3CF9F0. */
const char* LocationEventNames[0x201] =
{
"000) Guardia Throneroom",
"001) Bangor Dome",
"002)",
"003) Trann Dome",
"004) Arris Dome",
"005)",
"006) Arris Dome Auxiliary Console",
"007)",
"008)",
"009)",
"00A) Epoch Moonlight Parade Ending Zenan Bridge Scene",
"00B) Epoch Moonlight Parade Ending Magus Scene",
"00C)",
"00D)",
"00E)",
"00F)",
"010)",
"011)",
"012)",
"013)",
"014)",
"015) Sun Palace",
"016)",
"017)",
"018)",
"019)",
"01A) Lucca's Workshop",
"01B) Millenial Fair",
"01C) Telepod Exhibit",
"01D)",
"01E) Lucca's Room",
"01F) Crono's Trial",
"020)",
"021)",
"022) Denadoro Entrance",
"023)",
"024)",
"025) Magic Cave Exterior",
"026) Magic Cave",
"027) Factory Ruins Entrance",
"028)",
"029) Factory Ruins Security Center",
"02A) Factory Ruins Crane Room",
"02B)",
"02C)",
"02D)",
"02E)",
"02F)",
"030)",
"031)",
"032)",
"033) Death Peak Summit",
"034)",
"035)",
"036)",
"037)",
"038)",
"039) Tyrano Lair Lower Cell",
"03A) Lair Ruins",
"03B)",
"03C)",
"03D)",
"03E) Portal Cave (12000 BC)",
"03F)",
"040)",
"041)",
"042)",
"043)",
"044)",
"045) Title Screen",
"046) Title Screen",
"047)",
"048)",
"049)",
"04A) Leene Square",
"04B) Millenial Fair Moonlight Parade Ending",
"04C) Millenial Fair Moonlight Parade Ending",
"04D) Millenial Fair Moonlight Parade Ending",
"04E) Millenial Fair Moonlight Parade Ending",
"04F) Millenial Fair Moonlight Parade Ending",
"050) Millenial Fair Moonlight Parade Ending",
"051) Millenial Fair Moonlight Parade Ending",
"052) Guardia Cellar",
"053) Guardia Throneroom Moonlight Parade Ending",
"054)",
"055) Leene Square Moonlight Parade Ending",
"056)",
"057)",
"058)",
"059)",
"05A)",
"05B)",
"05C)",
"05D)",
"05E)",
"05F)",
"060)",
"061)",
"062)",
"063) Algetty Tidal Wave",
"064)",
"065)",
"066)",
"067)",
"068)",
"069)",
"06A)",
"06B)",
"06C)",
"06D)",
"06E)",
"06F) Gato's Exhibit",
"070) Prehistoric Exhibit",
"071)",
"072) Guardia Forest Portal",
"073)",
"074)",
"075)",
"076)",
"077)",
"078)",
"079)",
"07A)",
"07B)",
"07C) Tata's House",
"07D)",
"07E) Porre Elder's House",
"07F)",
"080)",
"081)",
"082)",
"083)",
"084)",
"085)",
"086)",
"087)",
"088)",
"089)",
"08A) Hunting Range",
"08B)",
"08C)",
"08D)",
"08E)",
"08F)",
"090)",
"091)",
"092)",
"093)",
"094)",
"095)",
"096)",
"097)",
"098)",
"099)",
"09A)",
"09B)",
"09C)",
"09D)",
"09E)",
"09F)",
"0A0)",
"0A1)",
"0A2)",
"0A3) Keeper's Dome Hanger",
"0A4)",
"0A5)",
"0A6)",
"0A7)",
"0A8)",
"0A9)",
"0AA)",
"0AB)",
"0AC)",
"0AD)",
"0AE)",
"0AF)",
"0B0)",
"0B1) Cathedral Chapel",
"0B2)",
"0B3)",
"0B4)",
"0B5)",
"0B6)",
"0B7)",
"0B8)",
"0B9) Cursed Woods",
"0BA) Frog's Burrow",
"0BB)",
"0BC) Masamune Cave",
"0BD)",
"0BE) Yakra's Chamber",
"0BF)",
"0C0)",
"0C1) Naga-ette Bromide Storage",
"0C2)",
"0C3)",
"0C4) Frog King Fight",
"0C5)",
"0C6)",
"0C7)",
"0C8)",
"0C9)",
"0CA)",
"0CB)",
"0CC) Zeal Throneroom",
"0CD)",
"0CE)",
"0CF) Enhasa",
"0D0) Enhasa",
"0D1) Kajar",
"0D2)",
"0D3)",
"0D4)",
"0D5)",
"0D6)",
"0D7)",
"0D8)",
"0D9)",
"0DA)",
"0DB)",
"0DC)",
"0DD)",
"0DE)",
"0DF)",
"0E0)",
"0E1)",
"0E2)",
"0E3) Crono's Sacrifice",
"0E4) Crono's Sacrifice",
"0E5) Crono's Sacrifice",
"0E6) Crono's Sacrifice",
"0E7)",
"0E8)",
"0E9)",
"0EA)",
"0EB)",
"0EC)",
"0ED)",
"0EE)",
"0EF)",
"0F0) Guardia Throneroom 600 AD",
"0F1)",
"0F2) Guardia Queen's Chambers 600 AD",
"0F3)",
"0F4)",
"0F5)",
"0F6) Castle Magus Entrance",
"0F7)",
"0F8)",
"0F9)",
"0FA)",
"0FB)",
"0FC)",
"0FD) Castle Magus Inner Sanctum",
"0FE)",
"0FF) Castle Magus Ozzie's Room",
"100)",
"101)",
"102)",
"103)",
"104)",
"105) Blackbird Left Wing",
"106)",
"107)",
"108)",
"109)",
"10A)",
"10B)",
"10C)",
"10D) Blackbird Cell",
"10E)",
"10F)",
"110)",
"111)",
"112)",
"113)",
"114)",
"115)",
"116)",
"117)",
"118)",
"119)",
"11A)",
"11B) End of Time",
"11C) Spekkio",
"11D)",
"11E)",
"11F)",
"120)",
"121)",
"122)",
"123)",
"124)",
"125)",
"126)",
"127) Lavos attacks Truce Dome",
"128) Black Omen rises",
"129)",
"12A) Tesseract",
"12B)",
"12C)",
"12D)",
"12E)",
"12F)",
"130)",
"131)",
"132)",
"133)",
"134)",
"135)",
"136)",
"137)",
"138)",
"139)",
"13A)",
"13B)",
"13C)",
"13D)",
"13E)",
"13F)",
"140)",
"141)",
"142)",
"143) Crono's Kitchen",
"144) Chrono's Room",
"145)",
"146)",
"147)",
"148)",
"149)",
"14A)",
"14B)",
"14C) Sun Keep 1000 AD",
"14D)",
"14E) Truce Canyon Portal",
"14F)",
"150) Banta the Blacksmith's house",
"151) Truce Inn 600 AD",
"152)",
"153)",
"154)",
"155)",
"156)",
"157)",
"158)",
"159) Lab 32 West Entrance",
"15A)",
"15B) Proto Dome",
"15C) Proto Dome Portal",
"15D)",
"15E)",
"15F)",
"160)",
"161) Sun Keep 12000 BC",
"162)",
"163)",
"164)",
"165)",
"166)",
"167) Geno Dome Mainframe",
"168)",
"169) Special purpose area (Location 10F) 6/25/2004",
"16A)",
"16B) Mystic Mtns Portal",
"16C) Mystic Mtns Reptite Pass",
"16D)",
"16E) Chief's Hut",
"16F)",
"170) Ioka Trading Post",
"171)",
"172) Ioka Meeting Site",
"173) Ioka Meeting Site Celebration",
"174)",
"175)",
"176)",
"177)",
"178)",
"179)",
"17A)",
"17B)",
"17C)",
"17D)",
"17E) Dactyl Nest Summit",
"17F)",
"180)",
"181)",
"182)",
"183)",
"184)",
"185)",
"186)",
"187)",
"188)",
"189)",
"18A)",
"18B)",
"18C)",
"18D)",
"18E)",
"18F)",
"190)",
"191)",
"192) Last Village Commons",
"193) Last Village Empty Hut",
"194) Last Village Shop",
"195)",
"196) North Cape",
"197)",
"198)",
"199)",
"19A) Prison Entrance",
"19B) Prison Execution Chamber",
"19C) Medina Square",
"19D)",
"19E) Medina Elder's House",
"19F)",
"1A0)",
"1A1) Medina Portal",
"1A2)",
"1A3)",
"1A4) Medina Merchant",
"1A5) Melchior's Hut",
"1A6)",
"1A7) Nu and the choice of weapons",
"1A8)",
"1A9)",
"1AA)",
"1AB)",
"1AC) Heckran's Lair",
"1AD)",
"1AE)",
"1AF) Fiona's Shrine",
"1B0)",
"1B1)",
"1B2)",
"1B3)",
"1B4)",
"1B5)",
"1B6)",
"1B7)",
"1B8)",
"1B9)",
"1BA)",
"1BB)",
"1BC) Prison Cells",
"1BD)",
"1BE)",
"1BF)",
"1C0)",
"1C1)",
"1C2)",
"1C3)",
"1C4)",
"1C5)",
"1C6)",
"1C7)",
"1C8)",
"1C9)",
"1CA) Black Omen Sky Gate",
"1CB) Black Omen Sky Gate",
"1CC) Black Omen Sky Gate",
"1CD) Black Omen Sky Gate",
"1CE) Black Omen Sky Gate",
"1CF) Black Omen Sky Gate",
"1D0)",
"1D1) Zenan Bridge 600 AD",
"1D2) Zenan Bridge 600 AD",
"1D3) Fiona's Villa",
"1D4)",
"1D5)",
"1D6)",
"1D7)",
"1D8)",
"1D9)",
"1DA)",
"1DB)",
"1DC)",
"1DD)",
"1DE)",
"1DF)",
"1E0)",
"1E1)",
"1E2)",
"1E3)",
"1E4)",
"1E5)",
"1E6)",
"1E7)",
"1E8)",
"1E9)",
"1EA) Forest Maze",
"1EB) Laruba Village",
"1EC)",
"1ED)",
"1EE)",
"1EF)",
"1F0)",
"1F1)",
"1F2)",
"1F3)",
"1F4)",
"1F5)",
"1F6)",
"1F7)",
"1F8) Beast's Nest",
"1F9)",
"1FA)",
"1FB)",
"1FC)",
"1FD)",
"1FE) Mount Woe Summit",
"1FF) Mount Woe Summit",
"200)"
};

const char* MapNames[0x200] =
{
    "{000} Load Screen",
    "{001} Crono's Kitchen",
    "{002} Crono's Room",
    "{003} Lucca's Kitchen",
    "{004} Lucca's Workshop",
    "{005} Millenial Fair",
    "{006} Gato's Exhibit",
    "{007} Prehistoric Exhibit",
    "{008} Telepod Exhibit",
    "{009} Lara's Room",
    "{00A} Lucca's Room",
    "{00B} Ending Selector",
    "{00C} Truce Inn (Present)",
    "{00D} Truce Mayor's Manor 1F",
    "{00E} Truce Mayor's Manor 2F",
    "{00F} Truce Single Woman Residence",
    "{010} Truce Happy Screaming Couple Residence",
    "{011} Truce Market (Present)",
    "{012} Truce Ticket Office",
    "{013} Guardia Forest (Present)",
    "{014} Guardia Forest Dead End",
    "{015} Guardia Throneroom (Present)",
    "{016} King's Chamber (Present)",
    "{017} Queen's Chamber (Present)",
    "{018} Guardia Kitchen (Present)",
    "{019} Guardia Barracks (Present)",
    "{01A} Guardia Basement",
    "{01B} Courtroom",
    "{01C} Prison Catwalks",
    "{01D} Prison Supervisor's Office",
    "{01E} Prison Torture Storage Room",
    "{01F} Medina Square",
    "{020} Zenan Bridge (Present)",
    "{021} Medina Elder's House 1F",
    "{022} Medina Elder's House 2F",
    "{023} Medina Inn",
    "{024} Medina Portal",
    "{025} Ending: Legendary Hero",
    "{026} Guardia Throneroom (Ending: Legendary Hero)",
    "{027} Medina Market",
    "{028} Melchior's Kitchen",
    "{029} Melchior's Workshop",
    "{02A} Forest Ruins",
    "{02B} Cursed Woods (Ending: The Unknown Past)",
    "{02C} Leene Square (Future)",
    "{02D} Denadoro Mountain Vista (Ending: Legendary Hero)",
    "{02E} Castle Magus Throne of Defense (Ending: Legendary Hero)",
    "{02F} Heckran Cave Passageways",
    "{030} Heckran Cave Entrance",
    "{031} Heckran Cave Underground River",
    "{032} Porre Mayor's Manor 1F (Present)",
    "{033} Porre Mayor's Manor 2F (Present)",
    "{034} Porre Residence (Present)",
    "{035} Snail Stop",
    "{036} Porre Market (Present)",
    "{037} Porre Inn (Present)",
    "{038} Porre Ticket Office",
    "{039} Fiona's Shrine",
    "{03A} Choras Mayor's Manor 1F",
    "{03B} Choras Mayor's Manor 2F",
    "{03C} Castle Magus Hall of Aggression (Ending: Legendary Hero)",
    "{03D} Choras Carpenter's Residence (Present)",
    "{03E} Choras Inn (Present)",
    "{03F} West Cape",
    "{040} Sun Keep (Present)",
    "{041} Northern Ruins Entrance (Present)",
    "{042} Northern Ruins Basement Corridor (Present)",
    "{043} Northern Ruins Landing (Present)",
    "{044} Northern Ruins Antechamber (Present)",
    "{045} Northern Ruins Vestibule (Present)",
    "{046} Northern Ruins Back Room (Present)",
    "{047} Prison Cells",
    "{048} Prison Stairwells",
    "{049} Northern Ruins Hero's Grave (Present)",
    "{04A} Unknown",
    "{04B} Prison Exterior",
    "{04C} Unknown (Lavos Spawn)",
    "{04D} Unknown (Lavos Spawn)",
    "{04E} Unknown (Lavos Spawn)",
    "{04F} Guardia Throneroom (Ending: Moonlight Parade)",
    "{050} Millenial Fair (Ending: Moonlight Parade)",
    "{051} Leene Square (Ending: Moonlight Parade)",
    "{052} Ending Selector",
    "{053} Fiona's Forest Recriminations",
    "{054} End of Time (Ending: Moonlight Parade)",
    "{055} Telepod Exhibit (Ending: Moonlight Parade)",
    "{056} Death Peak Summit (Ending: Moonlight Parade)",
    "{057} Manoria Sanctuary (Ending: The Successor of Guardia)",
    "{058} The End",
    "{059} Zenan Bridge (Ending: Moonlight Parade)",
    "{05A} Prison Catwalks (Ending: Moonlight Parade)",
    "{05B} Ending: People of the Times (Part 1)",
    "{05C} Ending: People of the Times (Part 2)",
    "{05D} Ending: People of the Times (Part 3)",
    "{05E} Ending: People of the Times (Part 4)",
    "{05F} Ending: People of the Times (Part 5)",
    "{060} Black Omen Lavos Spawn",
    "{061} Black Omen 3F Teleporter (no exits)",
    "{062} Black Omen 45F Teleporter",
    "{063} Black Omen Platform (no exit)",
    "{064} Black Omen Platform Shaft (Downward)",
    "{065} Black Omen Platform Shaft (Upward)",
    "{066} Black Omen Celestial Gate (no map)",
    "{067} Black Omen Celestial Gate (no map)",
    "{068} Black Omen Celestial Gate (no map)",
    "{069} Black Omen Celestial Gate (no map)",
    "{06A} Black Omen Celestial Gate (no map)",
    "{06B} Black Omen Celestial Gate",
    "{06C} Lucca Explains Paradoxes",
    "{06D} Ancient Tyrano Lair",
    "{06E} Ancient Tyrano Lair Traps",
    "{06F} Ancient Tyrano Lair Nizbel's Room",
    "{070} Truce Canyon",
    "{071} Truce Canyon Portal",
    "{072} Truce Couple's Residence (Middle Ages)",
    "{073} Truce Smithy's Residence",
    "{074} Truce Inn 1F (Middle Ages)",
    "{075} Truce Inn 2F (Middle Ages)",
    "{076} Truce Market (Middle Ages)",
    "{077} Guardia Forest (Middle Ages)",
    "{078} Guardia Throneroom (Middle Ages)",
    "{079} Guardia King's Chamber (Middle Ages)",
    "{07A} Guardia Queen's Chamber (Middle Ages)",
    "{07B} Guardia Kitchen (Middle Ages)",
    "{07C} Guardia Barracks (Middle Ages)",
    "{07D} Castle Magus Doppleganger Corridor",
    "{07E} Geno Dome Main Conveyor",
    "{07F} Geno Dome Elevator",
    "{080} Geno Dome Long Corridor",
    "{081} Manoria Sanctuary",
    "{082} Manoria Main Hall",
    "{083} Manoria Headquarters",
    "{084} Manoria Royal Guard Hall",
    "{085} Zenan Bridge (Wrecked)",
    "{086} Zenan Bridge (Middle Ages, no map)",
    "{087} Zenan Bridge (Middle Ages)",
    "{088} Sandorino Pervert Residence",
    "{089} Sandorino Elder's House",
    "{08A} Sandorino Inn",
    "{08B} Sandorino Market",
    "{08C} Cursed Woods",
    "{08D} Frog's Burrow",
    "{08E} Denadoro South Face",
    "{08F} Denadoro Cave of the Masamune Exterior",
    "{090} Denadoro North Face",
    "{091} Denadoro Entrance",
    "{092} Denadoro Lower East Face",
    "{093} Denadoro Upper East Face",
    "{094} Denadoro Mountain Vista",
    "{095} Denadoro West Face",
    "{096} Denadoro Gauntlet",
    "{097} Denadoro Cave of the Masamune",
    "{098} Tata's House 1F",
    "{099} Tata's House 2F",
    "{09A} Porre Elder's House (Middle Ages)",
    "{09B} Porre Cafe (Middle Ages)",
    "{09C} Porre Inn (Middle Ages)",
    "{09D} Porre Market (Middle Ages)",
    "{09E} Fiona's Villa",
    "{09F} Sunken Desert Entrance",
    "{0A0} Sunken Desert Parasytes",
    "{0A1} Sunken Desert Devourer",
    "{0A2} Ozzie's Fort Entrance (no map)",
    "{0A3} Magic Cave Exterior",
    "{0A4} Magic Cave Interior",
    "{0A5} Castle Magus Exterior",
    "{0A6} Castle Magus Entrance",
    "{0A7} Castle Magus Chamber of Guillotines",
    "{0A8} Castle Magus Chamber of Pits",
    "{0A9} Castle Magus Throne of Strength",
    "{0AA} Castle Magus Hall of Aggression",
    "{0AB} Castle Magus Hall of Deceit",
    "{0AC} Castle Magus Inner Sanctum",
    "{0AD} Castle Magus Throne of Magic",
    "{0AE} Castle Magus Throne of Defense",
    "{0AF} Castle Magus Hall of Apprehension",
    "{0B0} Castle Magus Lower Battlements",
    "{0B1} Ozzie's Fort Entrance",
    "{0B2} Ozzie's Fort Hall of Disregard",
    "{0B3} Ozzie's Fort Chamber of Kitchen Knives",
    "{0B4} Ozzie's Fort Last Stand",
    "{0B5} Ozzie's Fort Throne of Incompetence",
    "{0B6} Ozzie's Fort Throne of Impertinence (wrong map)",
    "{0B7} Ozzie's Fort Throne of Impertinence",
    "{0B8} Ozzie's Fort Throne of Ineptitude",
    "{0B9} Choras Old Couple Residence (Middle Ages)",
    "{0BA} Choras Carpenter's Residence 1F (Middle Ages)",
    "{0BB} Choras Carpenter's Residence 2F (Middle Ages)",
    "{0BC} Choras Cafe",
    "{0BD} Choras Inn (Middle Ages)",
    "{0BE} Choras Market (Middle Ages)",
    "{0BF} Sun Keep (Middle Ages)",
    "{0C0} (empty map)",
    "{0C1} (empty map)",
    "{0C2} (empty map)",
    "{0C3} Giant's Claw Entrance",
    "{0C4} Giant's Claw Caverns",
    "{0C5} Giant's Claw Last Tyranno",
    "{0C6} Manoria Command",
    "{0C7} Manoria Confinement",
    "{0C8} Manoria Shrine Antechamber",
    "{0C9} Manoria Storage",
    "{0CA} Manoria Kitchen",
    "{0CB} Manoria Shrine",
    "{0CC} Guardia Forest Frog King Battle",
    "{0CD} Denadoro Cyrus's Last Battle",
    "{0CE} Guardia Throneroom Cyrus's Final Mission",
    "{0CF} Schala's Room (no map)",
    "{0D0} Bangor Dome",
    "{0D1} Bangor Dome Sealed Room",
    "{0D2} Trann Dome",
    "{0D3} Trann Dome Sealed Room",
    "{0D4} Lab 16 West",
    "{0D5} Lab 16 East",
    "{0D6} Arris Dome",
    "{0D7} Arris Dome Infestation",
    "{0D8} Arris Dome Auxiliary Console",
    "{0D9} Arris Dome Lower Commons",
    "{0DA} Arris Dome Command Central",
    "{0DB} Arris Dome Guardian Chamber",
    "{0DC} Arris Dome Sealed Room",
    "{0DD} Arris Dome Rafters",
    "{0DE} Reptite Lair 2F",
    "{0DF} Lab 32 West Entrance",
    "{0E0} Lab 32",
    "{0E1} Lab 32 East Entrance",
    "{0E2} Proto Dome",
    "{0E3} Proto Dome Portal",
    "{0E4} Factory Ruins Entrance",
    "{0E5} Factory Ruins Auxiliary Console",
    "{0E6} Factory Ruins Security Center",
    "{0E7} Factory Ruins Crane Room",
    "{0E8} Factory Ruins Infestation",
    "{0E9} Factory Ruins Crane Control Room",
    "{0EA} Factory Ruins Information Archive",
    "{0EB} Factory Ruins Power Core",
    "{0EC} Sewer Access B1",
    "{0ED} Sewer Access B2",
    "{0EE} Ending: Dream Team (no map) (TBD)",
    "{0EF} Ending: A Slide Show?",
    "{0F0} Ending: Goodnight",
    "{0F1} Keeper's Dome",
    "{0F2} Keeper's Dome Corridor",
    "{0F3} Keeper's Dome Hanger",
    "{0F4} Death Peak Entrance",
    "{0F5} Death Peak South Face",
    "{0F6} Death Peak Southeast Face",
    "{0F7} Death Peak Northeast Face",
    "{0F8} Geno Dome Entrance",
    "{0F9} Geno Dome Conveyor Entrance",
    "{0FA} Geno Dome Conveyor Exit",
    "{0FB} Sun Palace",
    "{0FC} Millenial Fair (Ending: Dino Age)",
    "{0FD} Sun Keep (Last Village)",
    "{0FE} Skill Tutorial",
    "{0FF} Sun Keep (Future)",
    "{100} Geno Dome Labs",
    "{101} Geno Dome Storage",
    "{102} Geno Dome Robot Hub",
    "{103} Factory Ruins Data Core",
    "{104} Death Peak Northwest Face",
    "{105} Prehistoric Canyon",
    "{106} Death Peak Upper North Face",
    "{107} Death Peak Lower North Face",
    "{108} Death Peak Cave",
    "{109} Death Peak Summit",
    "{10A} Ending: Dream Team (no map) (TBD)",
    "{10B} Geno Dome Robot Elevator Access",
    "{10C} Geno Dome Mainframe",
    "{10D} Geno Dome Waste Disposal",
    "{10E} Leene's Square (Ending: Dino Age)",
    "{10F} Special Purpose Area",
    "{110} Mystic Mtn Portal",
    "{111} Mystic Mtn Base",
    "{112} Mystic Mtn Gulch",
    "{113} Chief's Hut",
    "{114} Ioka Southwestern Hut",
    "{115} Ioka Trading Post",
    "{116} Ioka Sweet Water Hut",
    "{117} Ioka Meeting Site",
    "{118} Ioka Meeting Site (Party)",
    "{119} Forest Maze Entrance",
    "{11A} Forest Maze",
    "{11B} Reptite Lair Entrance",
    "{11C} Reptite Lair 1F",
    "{11D} Reptite Lair Weevil Burrows B1",
    "{11E} Reptite Lair Weevil Burrows B2",
    "{11F} Reptite Lair Commons",
    "{120} Reptite Lair Tunnel",
    "{121} Reptite Lair Azala's Room",
    "{122} Reptite Lair Access Shaft",
    "{123} Hunting Range",
    "{124} Laruba Ruins",
    "{125} Dactyl Nest, Lower",
    "{126} Dactyl Nest, Upper",
    "{127} Dactyl Nest Summit",
    "{128} Giant's Claw Lair Entrance",
    "{129} Giant's Claw Lair Throneroom",
    "{12A} Tyrano Lair Exterior",
    "{12B} Tyrano Lair Entrance",
    "{12C} Tyrano Lair Throneroom",
    "{12D} Tyrano Lair Keep",
    "{12E} Tyrano Lair Antechambers",
    "{12F} Tyrano Lair Storage",
    "{130} Tyrano Lair Nizbel's Room",
    "{131} Tyrano Lair Room of Vertigo",
    "{132} Debug Room?",
    "{133} Lair Ruins Portal",
    "{134} Black Omen 1F Entrance",
    "{135} Black Omen 1F Walkway",
    "{136} Black Omen 1F Defense Corridor",
    "{137} Black Omen 1F Stairway",
    "{138} Black Omen 3F Walkway",
    "{139} Black Omen Auxilary Command",
    "{13A} Black Omen (TBD)",
    "{13B} Black Omen (TBD)",
    "{13C} Black Omen (TBD)",
    "{13D} Black Omen (TBD)",
    "{13E} Black Omen (TBD)",
    "{13F} Black Omen (TBD)",
    "{140} Black Omen (TBD)",
    "{141} Black Omen (TBD)",
    "{142} Black Omen (TBD)",
    "{143} Black Omen (TBD)",
    "{144} Black Omen (TBD)",
    "{145} Black Omen (TBD)",
    "{146} Black Omen (TBD)",
    "{147} Sunkeep (Prehistoric)",
    "{148} Zeal Palace Schala's Room",
    "{149} Zeal Palace Regal Hall",
    "{14A} Zeal Palace Corridor to the Mammon Machine",
    "{14B} Zeal Palace Hall of the Mammon Machine",
    "{14C} Zeal Palace Zeal Throneroom",
    "{14D} Zeal Palace Hall of the Mammon Machine (Night)",
    "{14E} Zeal Palace Zeal Throneroom (Night)",
    "{14F} Enhasa (wrong map)",
    "{150} Land Bridge (TBD)",
    "{151} Skyway? (TBD)",
    "{152} Present Age Home Room (Ending: The Dream Project)",
    "{153} Prehistoric Age Jungle (Ending: The Dream Project)",
    "{154} Dark Age Ocean Palace Room (Ending: The Dream Project)",
    "{155} Present Age Jail Cell (Ending: The Dream Project)",
    "{156} Dark Age Earthbound Room (Ending: The Dream Project)",
    "{157} Present Age Northern Ruins Room (Ending: The Dream Project)",
    "{158} Arris Dome Food Locker",
    "{159} Lucca's Workshop (Ending: The Oath)",
    "{15A} Arris Dome Guardian Chamber (Battle with Lavos)",
    "{15B} Prison Catwalks (Battle with Lavos)",
    "{15C} Heckran Cave (Battle with Lavos)",
    "{15D} Zenan Bridge (Battle with Lavos)",
    "{15E} Cave of the Masamune (Battle with Lavos)",
    "{15F} Dark Ages Portal",
    "{160} Land Bridge (TBD)",
    "{161} Land Bridge (TBD)",
    "{162} Skyway (TBD)",
    "{163} Enhasa",
    "{164} Skyway (TBD)",
    "{165} Kajar",
    "{166} Kajar Study",
    "{167} Kajar Belthasar's Private Room",
    "{168} Kajar Magic Lab",
    "{169} Zeal Palace Belthasar's Private Room",
    "{16A} Blackbird Scaffolding",
    "{16B} Blackbird Left Wing",
    "{16C} Blackbird Right Port",
    "{16D} Blackbird Left Port",
    "{16E} Blackbird Overhead",
    "{16F} Blackbird Hanger",
    "{170} Blackbird Rear Halls",
    "{171} Blackbird Forward Halls",
    "{172} Blackbird Treasury",
    "{173} Blackbird Cell",
    "{174} Blackbird Barracks",
    "{175} Blackbird Armory 3",
    "{176} Blackbird Inventory",
    "{177} Blackbird Lounge",
    "{178} Blackbird Ducts",
    "{179} Reborn Epoch",
    "{17A} Future Age Room (Ending: The Dream Project)",
    "{17B} End of Time (Ending: The Dream Project)",
    "{17C} Algetty",
    "{17D} Algetty Inn",
    "{17E} Algetty Elder's Grotto",
    "{17F} Algetty Commoner Grotto",
    "{180} Algetty Shop",
    "{181} Algetty Tsunami (wrong map)",
    "{182} Algetty Entrance",
    "{183} The Beast's Nest (wrong map)",
    "{184} The Beast's Nest",
    "{185} Zeal Teleporters",
    "{186} Prehistoric Hut (Ending: The Dream Project)",
    "{187} Castle Magus Room (Ending: The Dream Project)",
    "{188} Mt. Woe Western Face",
    "{189} Mt. Woe Lower Eastern Face",
    "{18A} Mt. Woe Middle Eastern Face",
    "{18B} Mt. Woe Upper Eastern Face",
    "{18C} Mt. Woe Summit (wrong map)",
    "{18D} Mt. Woe Summit",
    "{18E} Leene Square (Ending: What the Prophet Seeks)",
    "{18F} Crono's Kitchen (Ending: What the Prophet Seeks)",
    "{190} The End (Ending: Multiple?)",
    "{191} Zeal Palace",
    "{192} Zeal Palace Hallway",
    "{193} Zeal Palace Study",
    "{194} Ocean Palace Entrance",
    "{195} Ocean Palace Piazza",
    "{196} Ocean Palace Side Rooms",
    "{197} Ocean Palace Forward Area",
    "{198} Ocean Palace B3 Landing",
    "{199} Ocean Palace Grand Stairwell",
    "{19A} Ocean Palace B20 Landing",
    "{19B} Ocean Palace Southern Access Lift",
    "{19C} Ocean Palace Security Pool",
    "{19D} Ocean Palace Security Esplanade",
    "{19E} Ocean Palace Regal Antechamber",
    "{19F} Ocean Palace Throneroom",
    "{1A0} Ocean Palace (TBD)",
    "{1A1} Ocean Palace Eastern Access Lift",
    "{1A2} Ocean Palace Western Access Lift",
    "{1A3} Ocean Palace Time Freeze (wrong map)",
    "{1A4} Ocean Palace Time Freeze (wrong map)",
    "{1A5} Ocean Palace Time Freeze (wrong map)",
    "{1A6} Time Distortion Mammon Machine",
    "{1A7} Ocean Palace Time Freeze",
    "{1A8} Last Village Commons",
    "{1A9} Last Village Empty Hut",
    "{1AA} Last Village Shop",
    "{1AB} Last Village Residence",
    "{1AC} North Cape",
    "{1AD} Death Peak Summit",
    "{1AE} Tyranno Lair Main Cell",
    "{1AF} Title Screen (wrong map)",
    "{1B0} Flying Epoch",
    "{1B1} Title Screen",
    "{1B2} Bekkler's Lab",
    "{1B3} Magic Cave Exterior (after cutscene)",
    "{1B4} Fiona's Forest Campfire",
    "{1B5} Factory Ruins (TBD)",
    "{1B6} Courtroom King's Trial",
    "{1B7} Leene Square",
    "{1B8} Guardia Rear Storage",
    "{1B9} Courtroom Lobby",
    "{1BA} Blackbird Access Shaft",
    "{1BB} Blackbird Armory 2",
    "{1BC} Blackbird Armory 1",
    "{1BD} Blackbird Storage",
    "{1BE} Castle Magus Upper Battlements",
    "{1BF} Castle Magus Grand Stairway",
    "{1C0} (Bad Event Data Packet)",
    "{1C1} Black Omen Entrance",
    "{1C2} Black Omen Omega Defense",
    "{1C3} Black Omen Seat of Agelessness",
    "{1C4} (Bad Event Data Packet)",
    "{1C5} (Bad Event Data Packet)",
    "{1C6} (Bad Event Data Packet)",
    "{1C7} (Bad Event Data Packet)",
    "{1C8} Reptite Lair (Battle with Lavos)",
    "{1C9} Castle Magus Inner Sanctum (Battle with Lavos)",
    "{1CA} Tyrano Lair Keep (Battle with Lavos)",
    "{1CB} Mt. Woe Summit (Battle with Lavos)",
    "{1CC} Credits (TBD)",
    "{1CD} (Empty Data)",
    "{1CE} (Empty Data)",
    "{1CF} (Empty Data)",
    "{1D0} End of Time",
    "{1D1} Spekkio",
    "{1D2} Apocalypse Lavos",
    "{1D3} Lavos",
    "{1D4} Guardia Queen's Tower (Middle Ages)",
    "{1D5} Castle Magus Corridor of Combat",
    "{1D6} Castle Magus Hall of Ambush",
    "{1D7} Castle Magus Dungeon",
    "{1D8} Apocalypse Epoch",
    "{1D9} End of Time Epoch",
    "{1DA} Lavos Tunnel",
    "{1DB} Lavos Core",
    "{1DC} Truce Dome",
    "{1DD} Emergence of the Black Omen",
    "{1DE} Blackbird Wing Access",
    "{1DF} Tesseract",
    "{1E0} Guardia King's Tower (Middle Ages)",
    "{1E1} Death of the Blackbird",
    "{1E2} Blackbird (no exits)",
    "{1E3} Blackbird (no exits)",
    "{1E4} Blackbird (no exits)",
    "{1E5} Blackbird (no exits)",
    "{1E6} Guardia King's Tower (Present)",
    "{1E7} Guardia Queen's Tower (Present)",
    "{1E8} Guardia Lawgiver's Tower",
    "{1E9} Guardia Prison Tower",
    "{1EA} Ancient Tyrano Lair Room of Vertigo",
    "{1EB} (empty map)",
    "{1EC} (empty map)",
    "{1ED} Algetty Tsunami",
    "{1EE} Paradise Lost",
    "{1EF} Death Peak Guardian Spawn",
    "{1F0} Present",
    "{1F1} Middle Ages",
    "{1F2} Future",
    "{1F3} Prehistoric",
    "{1F4} Dark Ages",
    "{1F5} Kingdom of Zeal",
    "{1F6} Last Village",
    "{1F7} Apocalypse",
    "{1F8} (empty map)",
    "{1F9} (empty map)",
    "{1FA} (empty map)",
    "{1FB} (empty map)",
    "{1FC} (empty map)",
    "{1FD} (empty map)",
    "{1FE} (empty map)",
    "{1FF} (empty map)"
};
const char* Emotion[0x1B] =
{
"{00} Standing",
"{01} Walking",
"{02} Sprinting",
"{03} Battle",
"{04} Unknown",
"{05} Asleep",
"{06} Running",
"{07} Fast Running",
"{08} Defeated",
"{09} Shocked",
"{0A} Victory",
"{0B} Unknown",
"{0C} Determined",
"{0D} Unknown",
"{0E} Jump?",
"{0F} Unknown",
"{10} Shock?",
"{11} Standing?",
"{12} Weak",
"{13} Beat Chest (Robo)",
"{14} Unknown",
"{15} Right Hand Up",
"{16} Nod",
"{17} Shake Head",
"{18} Unknown",
"{19} D'oh!",
"{1A} Laugh     "
};
const char* NPCNames[0x100] =
{
"{00} Melchior",
"{01} King Guardia {1000 A.D.}",
"{02} Johnny",
"{03} Queen Leene {blue dress}",
"{04} Tata",
"{05} Toma",
"{06} Kino",
"{07} Chancellor {green} {1000 A.D.}",
"{08} Dactyl",
"{09} Schala",
"{0A} Janus",
"{0B} Chancellor {brown} {600 A.D.}",
"{0C} Belthasar",
"{0D} Middle Ages/Present Age villager - woman",
"{0E} Middle Ages/Present Age villager - young man",
"{0F} Middle Ages/Present Age villager - young woman",
"{10} Middle Ages/Present Age villager - soldier",
"{11} Middle Ages/Present Age villager - old man",
"{12} Middle Ages/Present Age villager - old woman",
"{13} Middle Ages/Present Age villager - little boy",
"{14} Middle Ages/Present Age villager - little girl",
"{15} Middle Ages/Present Age villager - waitress",
"{16} Middle Ages/Present Age villager - shopkeeper",
"{17} Nun",
"{18} Knight captain {600 A.D.}",
"{19} Middle Ages/Present Age villager - man",
"{1A} Dome survivor - man",
"{1B} Dome survivor - woman",
"{1C} Doan",
"{1D} Dome survivor - little girl",
"{1E} Prehistoric villager - man with club",
"{1F} Prehistoric villager - woman in green dress",
"{20} Prehistoric villager - little girl",
"{21} Prehistoric villager - old man",
"{22} Zeal citizen - man",
"{23} Zeal citizen - woman",
"{24} Zeal citizen - researcher with glasses",
"{25} Crono's mom",
"{26} Middle Ages/Present Age villager - little girl with purple hair",
"{27} Middle Ages/Present Age villager - man",
"{28} Middle Ages/Present Age villager - woman with purple hair",
"{29} Middle Ages/Present Age villager - young man",
"{2A} Middle Ages/Present Age villager - young woman",
"{2B} Middle Ages/Present Age villager - soldier",
"{2C} Middle Ages/Present Age villager - old man",
"{2D} Middle Ages/Present Age villager - old woman",
"{2E} Middle Ages/Present Age villager - little boy",
"{2F} Middle Ages/Present Age villager - little girl",
"{30} Middle Ages/Present Age villager - waitress with purple hair",
"{31} Middle Ages/Present Age villager - shopkeeper",
"{32} Nun",
"{33} Guardia knight {600 A.D.}",
"{34} Middle Ages/Present Age villager - man",
"{35} Cyrus",
"{36} Young Glenn",
"{37} King Guardia {600 A.D.}",
"{38} Strength Test Machine part {Millennial Fair}",
"{39} Middle Ages/Present Age villager - old man",
"{3A} Zeal citizen - researcher with glasses",
"{3B} Cat",
"{3C} False prophet Magus",
"{3D} Melchior in gray robe/NOT USED",
"{3E} Prehistoric villager - man carrying club with purple hair",
"{3F} Prehistoric villager - woman with purple hair",
"{40} Prehistoric villager - little girl with purple hair",
"{41} Algetty earthbound one - man",
"{42} Algetty earthbound one - woman",
"{43} Algetty earthbound one - old man",
"{44} Algetty earthbound one - child",
"{45} Queen Leene {green dress}",
"{46} Guardia Castle chef",
"{47} Trial judge",
"{48} Gaspar",
"{49} Fiona",
"{4A} Queen Zeal",
"{4B} Guard {enemy}",
"{4C} Reptite",
"{4D} Kilwala",
"{4E} Blue imp",
"{4F} Middle Ages/Present Age villager - man",
"{50} Middle Ages/Present Age villager - woman",
"{51} G.I. Jogger",
"{52} Millennial Fair visitor - old man",
"{53} Millennial Fair visitor - woman",
"{54} Millennial Fair visitor - little boy",
"{55} Millennial Fair visitor - little girl",
"{56} Lightning bolt",
"{57} Opened time portal - upper half",
"{58} Opened time portal - lower half",
"{59} Millennial Fair shopkeeper",
"{5A} Guillotine blade",
"{5B} Guillotine chain",
"{5C} Conveyor machine",
"{5D} Tombstone",
"{5E} Giant soup bowl",
"{5F} Magus statue",
"{60} Dreamstone",
"{61} Gate Key",
"{62} Soda can",
"{63} Pendant",
"{64} Poyozo doll",
"{65} Pink lunch bag",
"{66} UNUSED",
"{67} Red knife",
"{68} Broken Masamune blade",
"{69} Slice of cake",
"{6A} Trash can on its side",
"{6B} Piece of cheese",
"{6C} Barrel",
"{6D} UNUSED",
"{6E} Dead sunstone",
"{6F} Metal mug",
"{70} Blue star",
"{71} Giant blue star",
"{72} Red flame",
"{73} Giant red flame",
"{74} Explosion ball",
"{75} Giant explosion ball",
"{76} Smoke trail/NOT USED",
"{77} Hero's medal",
"{78} Balcony shadow",
"{79} Save point",
"{7A} Prehistoric villager - drummer",
"{7B} Prehistoric villager - log drummer",
"{7C} White explosion outline",
"{7D} Leene's bell",
"{7E} Bat hanging upside down",
"{7F} Computer screen",
"{80} Water splash",
"{81} Explosion",
"{82} Robo power-up sparks",
"{83} Leaves falling/NOT USED",
"{84} 3 coins spinning/NOT USED",
"{85} Hole in the ground",
"{86} Cooking smoke",
"{87} 3 small explosion clouds",
"{88} Wind element spinning",
"{89} Water element/NOT USED",
"{8A} Dirt mound",
"{8B} Masamune spinning",
"{8C} Music note/NOT USED",
"{8D} Dark candle/NOT USED",
"{8E} Water splash/NOT USED",
"{8F} Lightning bolt/NOT USED",
"{90} Unknown graphics/NOT USED",
"{91} UNUSED",
"{92} Small rock",
"{93} Small rock/NOT USED",
"{94} Small rock/NOT USED",
"{95} Rainbow shell",
"{96} Shadow {beds}",
"{97} Closed portal",
"{98} Balloon/NOT USED",
"{99} Light green bush",
"{9A} Shadow on the ground/NOT USED",
"{9B} Brown dreamstone/NOT USED",
"{9C} Crane machine",
"{9D} UNUSED",
"{9E} Dripping water/NOT USED",
"{9F} Cupboard doors",
"{A0} Brown stones/NOT USED",
"{A1} Dark green bush",
"{A2} Journal",
"{A3} Norstein Bekkler",
"{A4} Rat",
"{A5} Sparks from guillotine blade",
"{A6} Zeal teleporter",
"{A7} Ocean palace teleporter",
"{A8} Truce Dome director",
"{A9} Epoch seats",
"{AA} Robot",
"{AB} Red star",
"{AC} Sealed portal",
"{AD} Animated Zz {sleeping} icon",
"{AE} Flying map Epoch",
"{AF} Gray cat",
"{B0} Yellow cat",
"{B1} Alfador",
"{B2} Time egg",
"{B3} Zeal citizen - man {cast ending}",
"{B4} Zeal citizen - woman",
"{B5} Potted plant",
"{B6} Kid with purple hair {Glenn/Cyrus cutscene}",
"{B7} Sealed chest",
"{B8} Squirrel {Programmer's Ending}",
"{B9} Blue poyozo",
"{BA} Stone rubble pile",
"{BB} Rusted Robo",
"{BC} Gaspar {Gurus cutscene}",
"{BD} UNUSED",
"{BE} Orange cat",
"{BF} Middle Ages/Present Age villager - little boy",
"{C0} Middle Ages/Present Age villager - little girl",
"{C1} Spinning water element",
"{C2} Blue shining star - small",
"{C3} Blue shining star - large",
"{C4} Multiple balloons",
"{C5} Dancing woman {Millennial Fair ending}",
"{C6} Millennial Fair visitor - little girl",
"{C7} Silver Leene's bell",
"{C8} Figure atop Magus' Castle {ending}",
"{C9} Serving tray with drinks",
"{CA} THE END text",
"{CB} Human Glenn {ending cutscene}",
"{CC} Queen Zeal {Death Peak cutscene}",
"{CD} Schala {Death Peak cutscene}",
"{CE} Lavos {Death Peak cutscene}",
"{CF} Crono {Death Peak cutscene}",
"{D0} Hironobu Sakaguchi {Programmer's Ending}",
"{D1} Yuji Horii {Programmer's Ending}",
"{D2} Akira Toriyama {Programmer's Ending}",
"{D3} Kazuhiko Aoki {Programmer's Ending}",
"{D4} Lightning flash",
"{D5} Lara",
"{D6} Purple explosion",
"{D7} Crono's mom {Millennial Fair ending}",
"{D8} UNUSED",
"{D9} UNUSED",
"{DA} UNUSED",
"{DB} UNUSED",
"{DC} UNUSED",
"{DD} UNUSED",
"{DE} UNUSED",
"{DF} UNUSED",
"{E0} Green balloon/NOT USED",
"{E1} Yellow balloon/NOT USED",
"{E2} Blue balloon/NOT USED",
"{E3} Pink balloon/NOT USED",
"{E4} Brown glowing light/NOT USED",
"{E5} Yellow glowing light/NOT USED",
"{E6} Purple glowing light/NOT USED",
"{E7} Blue glowing light",
"{E8} UNUSED",
"{E9} UNUSED",
"{EA} UNUSED",
"{EB} UNUSED",
"{EC} UNUSED",
"{ED} UNUSED",
"{EE} UNUSED",
"{EF} UNUSED",
"{F0} UNUSED",
"{F1} UNUSED",
"{F2} UNUSED",
"{F3} UNUSED",
"{F4} UNUSED",
"{F5} UNUSED",
"{F6} UNUSED",
"{F7} UNUSED",
"{F8} UNUSED",
"{F9} UNUSED",
"{FA} UNUSED",
"{FB} UNUSED",
"{FC} UNUSED",
"{FD} UNUSED",
"{FE} UNUSED",
"{FF} UNUSED",
};
const char* SoundEffectNames[0x100] =
{
"{00} Cursor selection",
"{01} Invalid cursor selection",
"{02} Falling sprite",
"{03} Pendant reacting",
"{04} Received item",
"{05} Activating portal",
"{06} HP/MP restored",
"{07} End of Time HP/MP restore bucket",
"{08} Weapon readied",
"{09} Pendant reacting to Zeal throne room door",
"{0A} Flying object",
"{0B} Save point",
"{0C} UNUSED",
"{0D} UNUSED",
"{0E} UNUSED",
"{0F} UNUSED",
"{10} UNUSED",
"{11} UNUSED",
"{12} UNUSED",
"{13} UNUSED",
"{14} UNUSED",
"{15} UNUSED",
"{16} UNUSED",
"{17} UNUSED",
"{18} UNUSED",
"{19} UNUSED",
"{1A} UNUSED",
"{1B} UNUSED",
"{1C} UNUSED",
"{1D} UNUSED",
"{1E} UNUSED",
"{1F} UNUSED",
"{20} UNUSED",
"{21} UNUSED",
"{22} UNUSED",
"{23} UNUSED",
"{24} UNUSED",
"{25} UNUSED",
"{26} UNUSED",
"{27} UNUSED",
"{28} UNUSED",
"{29} UNUSED",
"{2A} UNUSED",
"{2B} UNUSED",
"{2C} UNUSED",
"{2D} UNUSED",
"{2E} UNUSED",
"{2F} UNUSED",
"{30} UNUSED",
"{31} UNUSED",
"{32} UNUSED",
"{33} UNUSED",
"{34} UNUSED",
"{35} UNUSED",
"{36} UNUSED",
"{37} UNUSED",
"{38} UNUSED",
"{39} UNUSED",
"{3A} UNUSED",
"{3B} UNUSED",
"{3C} UNUSED",
"{3D} UNUSED",
"{3E} UNUSED",
"{3F} Nothing?",
"{40} Curtains",
"{41} Wind [LOOPS]",
"{42} Machine engine [LOOPS]",
"{43} PC/NPC KO'd",
"{44} Machine powering up [LOOPS]",
"{45} Unknown",
"{46} Ocean waves [LOOPS]",
"{47} Bats flying [LOOPS]",
"{48} Unknown",
"{49} Long explosions",
"{4A} Stomach growling",
"{4B} Computer screen error [LOOPS]",
"{4C} Ferry horn",
"{4D} Enemy surprises party",
"{4E} Cat meow",
"{4F} Long fall",
"{50} Heavy sirens [LOOPS]",
"{51} Sealed door opening",
"{52} Switch pressed",
"{53} Door opened",
"{54} Earthquake/rumbling [LOOPS]",
"{55} Gold received",
"{56} Giant doors opened",
"{57} Metal mug put down",
"{58} Unknown [LOOPS] - NOT USED IN EVENTS",
"{59} Metal objects colliding 1",
"{5A} Metal objects colliding 2",
"{5B} Magic Urn enemy",
"{5C} Exhaust",
"{5D} Unknown [LOOPS] - NOT USED IN EVENTS",
"{5E} Conveyor belt [LOOPS]",
"{5F} Unknown",
"{60} Metal gate crashing",
"{61} Squeak",
"{62} Running [LOOPS]",
"{63} Weapon readied",
"{64} Poly rolling [LOOPS]",
"{65} Treasure chest opened",
"{66} Unknown",
"{67} Invalid password entry",
"{68} Crane password prompt",
"{69} Dactyl flying [LOOPS]",
"{6A} Unknown",
"{6B} Evil laugh",
"{6C} Machine malfunction [LOOPS]",
"{6D} Elevator moving [LOOPS]",
"{6E} Frog croak",
"{6F} Enemy scream 1",
"{70} Portal opening/closing",
"{71} Moving machine [LOOPS]",
"{72} Unknown",
"{73} Enemy scream 2",
"{74} Pathway opens",
"{75} Unknown - NOT USED IN EVENTS",
"{76} Unknown - NOT USED IN EVENTS",
"{77} Big explosion",
"{78} Teleport",
"{79} Unknown",
"{7A} NPC scream",
"{7B} Lightning on 2300 A.D. map",
"{7C} Thunder on 2300 A.D. map",
"{7D} Ground cracking before Lavos battle",
"{7E} Unknown",
"{7F} Unknown",
"{80} Rooster",
"{81} Unknown",
"{82} Metal bars rattling",
"{83} Guard KO'd",
"{84} Bushes/trees rustling",
"{85} Unknown [LOOPS]",
"{86} Telepod powering up",
"{87} Sword slice",
"{88} Unknown",
"{89} Transformation",
"{8A} Unknown - NOT USED IN EVENTS",
"{8B} Slice",
"{8C} Crashing metal",
"{8D} Sprite lands",
"{8E} Collision",
"{8F} Bat squeak",
"{90} Enemy scream",
"{91} Imp Ace flying",
"{92} Dragon Tank moving",
"{93} Ghosts [LOOPS]",
"{94} Unknown",
"{95} Unknown",
"{96} Bike race countdown",
"{97} Countdown start signal",
"{98} Robot noise",
"{99} Multiple explosions",
"{9A} Explosion",
"{9B} Ringing [LOOPS]",
"{9C} Enertron",
"{9D} Unknown",
"{9E} Computer display power on",
"{9F} Computer display power off",
"{A0} Typing",
"{A1} Light sirens [LOOPS]",
"{A2} Retinite moving [LOOPS]",
"{A3} Orb enemy blinking",
"{A4} Enemy scream 3",
"{A5} Trial audience cheers",
"{A6} Trial audience boos",
"{A7} Enemy sleeping",
"{A8} Pop",
"{A9} Unknown",
"{AA} Enemy startled",
"{AB} Water splash",
"{AC} Epoch preparing to warp",
"{AD} Epoch time warps",
"{AE} Epoch powering down",
"{AF} Epoch powering up [LOOPS]",
"{B0} Tonic obtained",
"{B1} Laughing",
"{B2} Lavos spawn scream",
"{B3} Crono obtains magic",
"{B4} Soldier walking [LOOPS]",
"{B5} Parried attack",
"{B6} Unknown",
"{B7} Hyper Kabob cooking",
"{B8} Digging",
"{B9} Unknown",
"{BA} Screen wipe",
"{BB} Machinery",
"{BC} Ozzie's barrier shattering",
"{BD} Ozzie falling",
"{BE} Masamune [LOOPS]",
"{BF} Masamune light beam [LOOPS]",
"{C0} Crane chain [LOOPS]",
"{C1} Unknown - NOT USED IN EVENTS",
"{C2} Unknown",
"{C3} Keeper's Dome Epoch warp",
"{C4} Robot computing 1",
"{C5} Tyrano roar",
"{C6} Robot computing 2",
"{C7} Robot computing slow",
"{C8} Epoch time warp",
"{C9} Teleport",
"{CA} Soda can bouncing",
"{CB} Blackbird cargo door opening  [LOOPS]",
"{CC} Blackbird cargo door opened",
"{CD} Unknown",
"{CE} Epoch firing laser[LOOPS]",
"{CF} Rapid explosions [LOOPS]",
"{D0} Epoch powering up [LOOPS]",
"{D1} Wormhole warp [LOOPS]",
"{D2} Epoch laser damaging Blackbird [LOOPS]",
"{D3} Robot computing 3",
"{D4} Large splash",
"{D5} Lavos beams destroying Zeal",
"{D6} Dinner chime",
"{D7} Power roast meal being prepared",
"{D8} Lavos breathing [LOOPS]",
"{D9} Epoch preparing to fly at Lavos [LOOPS]",
"{DA} Epoch flying into Lavos Mode 7 scene (3rd person side view) [LOOPS]",
"{DB} Epoch flying into Lavos Mode 7 scene (1st person) [LOOPS]",
"{DC} Epoch crashes into Lavos",
"{DD} Octo enemy",
"{DF} Light beams",
"{E0} Top of Black Omen [LOOPS]",
"{E1} Mammon Machine [LOOPS]",
"{E2} Lavos 2nd form [LOOPS]",
"{E3} Lavos 2nd form defeated",
"{E4} Unknown  - NOT USED IN EVENTS",
"{E5} Explosion engulfing Black Omen",
"{E6} Lavos eruption explosion",
"{E7} Computer analyzing map 1",
"{E8} Computer analyzing map 2 [LOOPS]",
"{E9} Ending slideshow [LOOPS]",
"{EA} ?",
"{EB} ?",
"{EC} ?",
"{ED} ?",
"{EE} ?",
"{EF} ?",
"{F0} ?",
"{F1} ?",
"{F2} ?",
"{F3} ?",
"{F4} ?",
"{F5} ?",
"{F6} ?",
"{F7} ?",
"{F8} ?",
"{F9} ?",
"{FA} ?",
"{FB} ?",
"{FC} ?",
"{FD} ?",
"{FE} ?",
"{FF} [END LOOP"
};

const char* SongNames[0x53] =
{
"{00} Silence",
"{01} 1.05 - Memories of Green",
"{02} 1.09 - Wind Scene",
"{03} 3.04 - Corridors of Time",
"{04} 2.20 - Rhythm of Wind, Sky, and Earth",
"{05} 2.01 - Ruined World",
"{06} 1.06 - Guardia Millenial Fair",
"{07} 3.09 - Far Off Promise",
"{08} 1.11 - Secret of the Forest",
"{09} 3.05 - Zeal Palace",
"{0A} 2.10 - Remains of Factory",
"{0B} 2.19 - Ayla's Theme",
"{0C} 1.13 - Courage and Pride",
"{0D} 2.05 - Lavos' Theme",
"{0E} 2.09 - Robo's Theme",
"{0F} 1.03 - Morning Sunlight",
"{10} 1.15 - Manoria Cathedral",
"{11} Sounds of the Ocean",
"{12} Leene's Bell",
"{13} 3.10 - Wings that Cross Time",
"{14} 3.06 - Schala's Theme",
"{15} 2.14 - Delightful Spekkio",
"{16} 1.23 - A Shot of Crisis",
"{17} 1.21 - Kingdom Trial",
"{18} 1.02 - Chrono Trigger",
"{19} Alert",
"{1A} Tsunami",
"{1B} 1.20 - Fanfare 1",
"{1C} 2.12 - Fanfare 2",
"{1D} 3.03 - At the Bottom of the Night",
"{1E} 1.04 - Peaceful Day",
"{1F} 1.08 - A Strange Happening",
"{20} Dungeon dripping noise",
"{21} Running Water",
"{22} Wind",
"{23} 1.22 - The Hidden Truth",
"{24} 1.16 - A Prayer to the Road that Leads",
"{25} 1.14 - Huh?",
"{26} 2.06 - The Day the World Revived",
"{27} 2.07 - Robo Gang Johnny",
"{28} 2.24 - Battle with Magus",
"{29} 1.18 - Boss Battle 1",
"{2A} 1.19 - Frog's Theme",
"{2B} 1.10 - Goodnight",
"{2C} 2.08 - Bike Chase",
"{2D} 2.04 - People who Threw Away the Will to Live",
"{2E} 2.02 - Mystery of the Past",
"{2F} 2.16 - Underground Sewer",
"{30} 1.01 - Presentiment",
"{31} 3.08 - Undersea Palace",
"{32} 3.14 - Last Battle",
"{33} 2.03 - Lab 16's Ruins",
"{34} Inside the Shell",
"{35} Quake",
"{36} 2.21 - Burn!  Bobonga!",
"{37} Wormhole",
"{38} 2.18 - Primitive Mountain",
"{39} 3.13 - World Revolution",
"{3A} Lavos Scream",
"{3B} 3.07 - Sealed Door",
"{3C} 1.17 - Silent Light",
"{3D} 2.15 - Fanfare 3",
"{3E} 2.13 - The Brink of Time",
"{3F} 3.17 - To Far Away Times",
"{40} 2.23 - Confusing Melody",
"{41} Hail Magus",
"{42} 1.07 - Gonzales' Song",
"{43} Rain",
"{44} 3.11 - Black Dream",
"{45} 1.12 - Battle 1",
"{46} 3.02 - Tyran Castle",
"{47} Fall of Mount Woe",
"{48} 2.22 - Magus' Castle",
"{49} 3.15 - First Festival of Stars",
"{4A} The Destruction of Zeal",
"{4B} Ocean Tide",
"{4C} Lavos' Breath",
"{4D} 3.16 - Epilogue - To Good Friends",
"{4E} 2.17 - Boss Battle 2",
"{4F} Fanfare 4",
"{50} 3.12 - Determination",
"{51} 2.11 - Battle 2",
"{52} 3.01 - Singing Mountain"
};

namespace
{
    struct paramholder: public std::wstring
    {
    private:
        void operator=(unsigned n);
        void operator=(unsigned char n);
        void operator=(char n);
    public:
        paramholder& operator= (const std::wstring& s)
        { std::wstring::operator=(s); return *this; }
    };
    typedef std::map<std::string, paramholder> parammap;

    /* Prints the string using the format, filling in the parameters. */
    static const std::wstring FormatString
        (const char*const format, const parammap& params)
    {
        std::wstring result;
        const char* fmtptr = format;
        while(*fmtptr)
        {
            if(*fmtptr == '%')
            {
                std::string paramname;
                while(std::isalnum(*++fmtptr)) paramname += *fmtptr;
                
                parammap::const_iterator i = params.find(paramname);
                if(i == params.end())
                {
                    std::fprintf(stderr, "Internal error: Param '%s' not found\n",
                        paramname.c_str());
                    continue;
                }
                result += i->second;
                continue;
            }
            result += WcharToAsc(*fmtptr++);
        }
        return result;
    }

    /* Formatting functions */
    static const std::wstring FormatNumeric(unsigned n, unsigned bits)
    {
        // Vars starting with Object are related to the actor.
        // Vars starting with Sprite are related to the GFX sprite.
        switch(n)
        {
            case 0x7E016D: return L"{ObjectID}";
            case 0x7E01BD: return L"{NumActors}";
            case 0x7E011F: return L"{ExploreMode}";
            case 0x7E0154: return L"{Unknown54}";
            case 0x7E016B: return L"{Unknown6B}";
            case 0x7E0197: return L"{Member1ObjectNo}";
            case 0x7E0199: return L"{Member2ObjectNo}";
            case 0x7E019B: return L"{Member3ObjectNo}";
            case 0x7E01F8: return L"{RandomCounter}";
            case 0x7E0520: return L"{Sprite520}";
            case 0x7E0521: return L"{Sprite521}"; // related to palette
            case 0x7E0522: return L"{Sprite522}";
            case 0x7E0523: return L"{Sprite523}";
            case 0x7E0524: return L"{Sprite524}";
            case 0x7E0525: return L"{Sprite525}";
            case 0x7E0526: return L"{Sprite526}";
            case 0x7E0527: return L"{Sprite527}";
            case 0x7E0528: return L"{Sprite528}";
            // 7E0F00: ?
            case 0x7E0F81: return L"{ObjectPaletteNumber}";
            // 1000: bitmask of unknown purpose (#$80 is a bit, lower bits are value)
            // 1001: maybe a copy of 1000 (see op 87)
            case 0x7E1100: return L"{ObjectMemberIdentity}";
            // 1100: Identity as a party member
            //         If bit $80 is set, the object is dead
            //         and its code will not be interpreted.
            //         #0: member1
            //         #1: member2
            //         #2: member3
            //         #3: out-party PC
            //         #4: NPC
            //         #5: monster
            case 0x7E1101: return L"{ObjectPlayerIdentity}";
            // 1101: Identity as a player character
            //         #0: crono
            //         #1: marle
            //         and so on
            case 0x7E1180: return L"{ObjectCodePointer}";
            // 1180: object's current code pointer
            // 1301: static animation? (like sleeping)
            case 0x7E1400: return L"{ObjectPalettePointer}";
            // 1400: Pointer to palette in ROM (offset only, page E4)
            // 7E15C0: ?
            case 0x7E1600: return L"{ObjectFacing}";
            // 1600: facing
            // 1601: possibly a L"facing is up to date" flag
            // 1680: current animation
            // 1681: possibly a L"animation is up to date" flag
            // 1780: ?flag
            // 1781: ?
            // 1800: ?flag for x-coord
            case 0x7E1801: return L"{ObjectXCoord}";
            // 1801: X-coordinate
            // 1880: ?flag for y-coord
            case 0x7E1881: return L"{ObjectYCoord}";
            // 1881: Y-coordinate
            // 1900: ?
            // 1980: ?
            case 0x7E1A00: return L"{ObjectSpeed}";
            // 1A00: NpcSpeed
            case 0x7E1A01: return L"{ObjectMovementLength}";
            // 1A01: Length of movement
            // 1A80: Appears to be a L"is moving?" flag
            case 0x7E1A81: return L"{ObjectDrawingMode}";
            // 1A81: Allocated? Drawing mode? 1=on, 0=off, $80=hide
            case 0x7E1B01: return L"{ObjectSolidProps}";
            // 1B01: NpcSolidProps
            // 1B80: ?flag
            // 1B81: ?
            case 0x7E1C00: return L"{ObjectPriorityNumber}";
            // 1C00: Current Priority number
            case 0x7E1C01: return L"{ObjectEventFlag}";
            // 1C01: EventFlag
            case 0x7E1C80: return L"{ObjectMoveProps}";
            // 1C80: NpcMoveProps
            // 1C81: ?
            //case 0x7F0200: return L"{DialogTextParam0}";
            //case 0x7F0201: return L"{DialogTextParam1}";
            //case 0x7F0202: return L"{DialogTextParam2}";
            //case 0x7F0203: return L"{DialogTextParam3}";
            //case 0x7F0204: return L"{DialogTextParam4}";
            //case 0x7F0205: return L"{DialogTextParam5}";
            // These may be used for dialog params, but
            // they are also used for various other purposes
            // as temporary variables.
            
            case 0x7F0580: return L"{ObjectPriority0Ptr}";
            // 7F0580: Priority 0 code pointer (begins as 0)
            case 0x7F0600: return L"{ObjectPriority1Ptr}";
            // 7F0600: Priority 1 code pointer (begins as 0)
            case 0x7F0680: return L"{ObjectPriority2Ptr}";
            // 7F0680: Priority 2 code pointer (begins as 0)
            case 0x7F0700: return L"{ObjectPriority3Ptr}";
            // 7F0700: Priority 3 code pointer (begins as 0)
            case 0x7F0780: return L"{ObjectPriority4Ptr}";
            // 7F0780: Priority 4 code pointer (begins as 0)
            case 0x7F0800: return L"{ObjectPriority5Ptr}";
            // 7F0800: Priority 5 code pointer (begins as 0)
            case 0x7F0880: return L"{ObjectPriority6Ptr}";
            // 7F0880: Priority 6 code pointer (begins as 0)
            case 0x7F0900: return L"{ObjectPriority7Ptr}";
            // 7F0900: Priority 7 code pointer (begins as 0)
            // 7F0980: flag used by opcode $04
            case 0x7E2980: return L"{Member1ID}";
            case 0x7E2981: return L"{Member2ID}";
            case 0x7E2982: return L"{Member3ID}";
            case 0x7E2983: return L"{Member4ID}";
            case 0x7E2984: return L"{Member5ID}";
            case 0x7E2985: return L"{Member6ID}";
            case 0x7E2986: return L"{Member7ID}";
            case 0x7E2987: return L"{Member8ID}";
            case 0x7E2988: return L"{Member9ID}";
            case 0x7F0000: return L"{StoryLineCounter}";
            case 0x7F0A80: return L"{Result}";
            // 7F0A80: "Result" of various tests
            // 7F0B01: Used by op B7
            // 7F0B80: Current pose number
        }
        return wformat(L"%0*X", bits/4, n);
    }

    static const std::wstring FormatDialogBegin(unsigned n, unsigned& save_begin)
    {
        save_begin = n = ROM2SNESaddr(n);
        return L"$" + AscToWstr(EncodeBase62(n, 4));
    }
    
    static const std::wstring FormatDialogAddr(unsigned n, unsigned saved_begin)
    {
        unsigned DialogAddr = saved_begin + n*2;
        return L"$" + AscToWstr(EncodeBase62(DialogAddr, 4));
    }

    static const std::wstring FormatOperator(unsigned char op)
    {
        static const wchar_t* ops[8] = {L"==", L"!=",
                                        L">",  L"<",
                                        L">=", L"<=",
                                        L"&",  L"|"};
        return ops[op & 7];
    }
    
    static const std::wstring FormatBlob(const std::vector<Byte>& data, bool has_text)
    {
        std::wstring result;
        result += '"';
        for(unsigned a=0; a<data.size(); ++a)
        {
            unsigned int byte = data[a];
            if(has_text)
            {
                if((byte == 1 || byte == 2) && (a+1 < data.size()))
                {
                    byte = byte*256 + data[++a];
                }
                wchar_t c = getwchar_t((ctchar)byte, cset_8pix);
                if(c != ilseq)
                {
                    result += c;
                    continue;
                }
            }
            result += wformat(L"[%02X]", byte);
        }
        result += '"';
        return result;
    }
}

#if 0
/* EV Parameter Handling Functions */
class EvParameterHandler
{
public:
    struct labeldata
    {
        std::wstring label_name;
        /* Decoding: */
        unsigned    label_value;
        /* Encoding: */
        unsigned    label_position;
        bool        label_forward;
    };

private:
    const char* opformat;
    unsigned    dataptr;
    labeldata label;
    unsigned    dialog_begin;
    
    struct structure
    {
        unsigned size;
        typedef std::map<std::wstring, elemdata> elemmap;
        elemmap elems;
    } structure;
    
protected:
    typedef std::map<std::string, paramholder> parammap;
    
    /* Throws a dummy exception if fails. */
    static unsigned ScanInt(const std::wstring& n, long max=0xFFFFFF)
    {
        unsigned offset=0;
        int base=16;
        if(n.substr(0, 2) == L"0x") { offset=2; base=16; }
        else if(n.substr(0, 1) == L"$") { offset=1; base=16; }
        char* endptr;
        long retval = std::strtol(n.c_str()+offset, &endptr, base);
        if(*endptr || retval < 0 || retval > max) throw false;
        return retval;
    }
    static const std::vector<Byte> ScanData(const std::wstring& n)
    {
        std::vector<Byte> result;
        if(n.size() < 2) throw false;
        if(n[0] != '"' || n[n.size()-1] != '"') throw false;
        
        for(unsigned a=1; a<n.size(); ++a)
        {
            if(n[a] == '[')
            {
                ++a;
                if(a+2 >= n.size())throw false;
                if(n[a+2] != ']') throw false;
                unsigned char code = ScanInt(n.substr(a+1,2), 0xFF);
                result.push_back(code);
            }
            else // Plain characters not yet handled.
                throw false;
        }
        return result;
    }
    const int ScanGoto(const std::wstring& n, unsigned bytepos, unsigned offset, int sign)
    {
        label.label_name     = n;
        label.label_position = bytepos;
        label.label_forward  = sign>0;
        
        return -offset;
    }
    static unsigned ScanOperator(const std::wstring& n)
    {
        if(n == L"==" || n == L"=") return 0;
        if(n == L"!=" || n == L"<>") return 1;
        if(n == L">") return 2;
        if(n == L"<") return 3;
        if(n == L">=") return 4;
        if(n == L"<=") return 5;
        if(n == L"&") return 6;
        if(n == L"|") return 7;
        throw false;
    }
    const unsigned ScanDialogBegin(const std::wstring& n)
    {
        if(n.size() != 5 || n[0] != '$') throw false;
        unsigned result=0;
        for(unsigned a=1; a<n.size(); ++a)
            if(!CumulateBase62(result, n[a])) throw false;
        
        dialog_begin = result;
        return SNES2ROMaddr(result);
    }
    const unsigned ScanDialogAddr(const std::wstring& n)
    {
        if(n.size() != 5 || n[0] != '$') throw false;
        unsigned result=0;
        for(unsigned a=1; a<n.size(); ++a)
            if(!CumulateBase62(result, n[a]))
                throw false;
        
        result -= dialog_begin;
        if(result & 1) throw false;
        result >>= 1;
        if(result > 0xFF) throw false;
        return result;
    }

public:
    /* Tests if the given string matches the given format. Returns true if ok. */
    bool CompareFormat(const std::string& string) const
    {
        parammap dummy;
        return ScanParams(string, dummy);
    }
    
    /* Attempts to scan the parameters from the string. Return false if fail. */
    bool ScanParams(const std::string& string, parammap& params) const
    {
        unsigned strpos=0;
        const char* fmtptr = opformat;
        while(strpos < string.size())
        {
            if(string[strpos] == ' ') { ++strpos; continue; }
            while(*fmtptr == ' ') ++fmtptr;
            if(*fmtptr == '%')
            {
                std::wstring paramname;
                while(std::isalnum(*++fmtptr)) paramname += *fmtptr;
                unsigned param_begin = strpos;
                
                enum { mode_unknown,
                       mode_number,
                       mode_operator,
                       mode_dialogaddr,
                       mode_identifier,
                       mode_string } mode = mode_unknown;
                
                for(; strpos < string.size(); ++strpos)
                {
                    char ch = string[strpos];
                    
                    bool ok=true;
                    switch(mode)
                    {
                        case mode_number:
                            if(!std::isxdigit(ch)) ok=false;
                            break;
                        case mode_dialogaddr:
                        case mode_identifier:
                            if(!std::isalnum(ch)) ok=false;
                            break;
                        case mode_operator: operator_check:
                            if(ch!='&' && ch!='|' && ch!='<' && ch!='>' && ch!='=' && ch!='!')
                                ok=false;
                            break;
                        case mode_string:
                            if(ch=='"') {++strpos;ok=false; }
                            break;
                        case mode_unknown:
                            if(ch=='"') {mode=mode_string;break;}
                            if(ch=='$') {mode=mode_dialogaddr;break;}
                            if(std::isxdigit(ch)) {mode=mode_number; break;}
                            if(std::isalpha(ch)) {mode=mode_identifier; break; }
                            mode=mode_operator;
                            goto operator_check;
                    }
                    if(!ok) break;
                }
                params[paramname] = string.substr(param_begin, strpos-param_begin);
                continue;
            }
            if(*fmtptr != string[strpos]) return false;
        }
        while(*fmtptr == ' ') ++fmtptr;
        return *fmtptr == '\0';
    }
};
#endif

namespace
{
    template<typename keytype>
    struct range
    {
        keytype lower, upper;
        
        //range(keytype v): lower(v),upper(v) {}
        range(keytype l,keytype u): lower(l),upper(u) {}
        
        /*bool operator< (keytype b) const
        {
            return upper < b;
        }*/
        bool operator< (const range& b) const
        {
            return lower < b.lower;
        }
        /*bool operator== (keytype b) const
        {
            return Contains(b);
        }*/
        bool operator== (const range& b) const
        {
            return lower==b.lower && upper==b.upper;
        }
        
        bool Contains(keytype b) const { return lower<=b && upper>=b; }
    };
    template<typename keytype, typename valuetype>
    class simple_rangemap: public std::multimap<range<keytype>, valuetype>
    {
    public:
    };
}

class EvCommands
{
private:
    class ElemData
    {
    public:
        enum typetype
        {
            t_trivial,
            t_nibble_hi,
            t_nibble_lo,
            t_orbit,
            t_andbit,
            t_else,
            t_loop,
            t_if,
            t_operator,
            t_textblob,
            t_blob,
            t_dialogbegin,
            t_dialogaddr
        };
    
        ElemData(unsigned nb, unsigned mi,unsigned ma,unsigned ad,int sh)
            : bytepos(0), type(t_trivial),
              n_bytes(nb),min(mi),max(ma),add(ad),shift(sh),
              highbit_trick(false) {}
        
        ElemData& DeclareHighbit()
        {
            highbit_trick = true;
            return *this;            
        }
        
        ElemData& SetBytePos(unsigned n) { bytepos = n; return *this; }
        
        ElemData& SetType(typetype t)
        {
            type   = t;
            return *this;
        }
        
        bool KnowRange(unsigned pos) const
        {
            /* Returns true if GetMin() and GetMax() can be used */
            if(n_bytes >= 1 && n_bytes <= 3
            && pos >= bytepos && pos < bytepos+n_bytes) return true;
            return false;
        }
        unsigned GetMin(unsigned pos) const
        {
            /* Return the minimum value an accepted byte
             * may have at the given position */
            switch(n_bytes)
            {
                case 1:
                    return min | (highbit_trick?0x80:0x00);
                case 2:
                {
                    unsigned min_hi = min >> 8, max_hi = max >> 8;
                    if(pos == bytepos+1) return min_hi | (highbit_trick?0x80:0x00);
                    if(min_hi != max_hi) return 0x00;
                    return min & 0xFF;
                }
                case 3:
                {
                    unsigned min_hi = min >> 16, max_hi = max >> 16;
                    if(pos == bytepos+2) return min_hi | (highbit_trick?0x80:0x00);
                    if(min_hi != max_hi) return 0x00;
                    min_hi = (min >> 8)&0xFF, max_hi = (max >> 8)&0xFF;
                    if(pos == bytepos+1) return min_hi | (highbit_trick?0x80:0x00);
                    if(min_hi != max_hi) return 0x00;
                    return min & 0xFF;
                }
            }
            return 0x00;
        }
        unsigned GetMax(unsigned pos) const
        {
            /* Return the maximum value an accepted byte
             * may have at the given position */
            switch(n_bytes)
            {
                case 1:
                    return max | (highbit_trick?0x80:0x00);
                case 2:
                {
                    unsigned min_hi = min >> 8, max_hi = max >> 8;
                    if(pos == bytepos+1) return max_hi | (highbit_trick?0x80:0x00);
                    if(min_hi != max_hi) return 0xFF;
                    return max & 0xFF;
                }
                case 3:
                {
                    unsigned min_hi = min >> 16, max_hi = max >> 16;
                    if(pos == bytepos+2) return max_hi | (highbit_trick?0x80:0x00);
                    if(min_hi != max_hi) return 0xFF;
                    min_hi = (min >> 8)&0xFF, max_hi = (max >> 8)&0xFF;
                    if(pos == bytepos+1) return max_hi | (highbit_trick?0x80:0x00);
                    if(min_hi != max_hi) return 0xFF;
                    return max & 0xFF;
                }
            }
            return 0xFF;
        }
        
        const std::wstring Format() const
        {
            if(type != t_trivial)
            {
                fprintf(stderr, "type(%u), not trivial\n", type);
                throw false;
            }
            
            return FormatNumeric(add, n_bytes*8);
        }
        
        struct FormatResult
        {
            std::wstring text;
            unsigned maxoffs;
            
            unsigned            goto_target;
            EventCode::gototype goto_type;
            
        public:
            FormatResult() : maxoffs(0),goto_target(0),goto_type(EventCode::goto_none) { }
        };
            
        FormatResult Format
            (unsigned offs, const unsigned char* data, unsigned maxlen,
             EventCode::DecodingState& state) const
        {
            FormatResult result;
            
            if(maxlen < bytepos)
            {
                fprintf(stderr, "maxlen(%u) bytepos(%u)\n", maxlen, bytepos);
                throw false;
            }
            maxlen -= bytepos; data += bytepos;
            
            switch(type)
            {
                case t_trivial:
                {
                    /* First, read the integer value. */
                    if(maxlen < n_bytes)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned value = 0;
                    for(unsigned n=0; n<n_bytes; ++n) value |= data[n] << (n*8);
                    
                    /* When highbit trick is used, there are two rules:
                     * - The input _must_ have high bit set
                     * - The interpreted value must _not_ have it.
                     */
                    if(highbit_trick)
                    {
                        unsigned mask = 1 << (n_bytes * 8 - 1);
                        if(!(value & mask))
                        {
                            fprintf(stderr, "value(%X) & mask(%X)\n", value, mask);
                            throw false;
                        }
                        value &= ~mask;
                    }
                    
                    /* Check ranges. */
                    if(value < min || value > max)
                    {
                        fprintf(stderr, "value(%u) min(%u) max(%u)\n", value, min, max);
                        throw false;
                    }
                    /* Adjust for formatting. */
                    if(shift < 0) value >>= -shift; else value <<= shift;
                    value += add;
                    /* Format. */
                    result.text    = FormatNumeric(value, n_bytes*8);
                    result.maxoffs = bytepos+n_bytes;
                    break;
                }
                case t_nibble_hi:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned value = data[0] >> 4;
                    result.text    = FormatNumeric(value, 4);
                    result.maxoffs = bytepos+1;
                    break;
                }
                case t_nibble_lo:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned value = data[0] & 15;
                    result.text    = FormatNumeric(value, 4);
                    result.maxoffs = bytepos+1;
                    break;
                }
                case t_orbit:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned value = 1 << (data[0]&7);
                    result.text    = FormatNumeric(value, 8);
                    result.maxoffs = bytepos+1;
                    break;
                }
                case t_andbit:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned value = (1 << (data[0]&7)) ^ 0xFF;
                    result.text    = FormatNumeric(value, 8);
                    result.maxoffs = bytepos+1;
                    break;
                }
                case t_else:
                case t_loop:
                case t_if:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    
                    int sign = type==t_loop ? -1 : 1;
                    
                    result.maxoffs     = bytepos+1;
                    result.goto_target = offs + data[0]*sign + bytepos;
                    result.goto_type   = type==t_loop ? EventCode::goto_backward
                                       : type==t_else ? EventCode::goto_forward
                                       : EventCode::goto_if;
                    break;
                }
                case t_operator:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned value = data[0];
                    result.text    = FormatOperator(value);
                    result.maxoffs = bytepos+1;
                    break;
                }
                case t_textblob:
                case t_blob:
                {
                    if(maxlen < 2)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned length = data[0] | (data[1] << 8);
                    data += 2; maxlen -= 2;
                    if(length < 2) throw false;
                    length -= 2;
                    if(maxlen < length)
                    {
                        fprintf(stderr, "blob: maxlen(%u) < length(%u)\n", maxlen, length);
                        throw false;
                    }
                    std::vector<Byte> buf(data, data+length);
                    result.text    = FormatBlob(buf, type==t_textblob);
                    result.maxoffs = bytepos+2+length;
                    break;
                }
                case t_dialogbegin:
                {
                    if(maxlen < 3)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned offs = data[0] | (data[1] << 8) | (data[2] << 16);
                    result.text    = FormatDialogBegin(offs, state.dialogbegin);
                    result.maxoffs = bytepos + 3;
                    break;
                }
                case t_dialogaddr:
                {
                    if(maxlen < 1)
                    {
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
                        throw false;
                    }
                    unsigned offs = data[0];
                    result.text    = FormatDialogAddr(offs, state.dialogbegin);
                    result.maxoffs = bytepos + 1;
                    break;
                }
            }
            return result;
        }
    private:
        unsigned bytepos;
        
        typetype type;
        
        // byte, word, long, memory, dialog begin, dialog addr:
        unsigned n_bytes; // example: 1
        unsigned min;  // example: 0x00
        unsigned max;  // example: 0xFF
        unsigned add;  // example: 0x7F0200
        int      shift;// -2=/4, -1=/2, 0=*1, 1=*2, 2=*4
        
        bool highbit_trick;
        
        // nibble:
        
        // else,loop,if:
        
        // operator:
        
        // blob:
    };
    
private:
    class OpcodeTree;
    
    class Command
    {
    public:
        Command() {}
        explicit Command(const char* fmt) : format(fmt) { }
    
        void Add(ElemData data, unsigned bytepos, const char* name)
        {
            data.SetBytePos(bytepos);
            pos_data.push_back(std::make_pair(*name?name:NULL, data));
        }
        void Add(ElemData data, unsigned bytepos)
        {
            data.SetBytePos(bytepos);
            pos_data.push_back(std::make_pair((const char*)NULL, data));
        }
        void Add(const ElemData& data, const char* name)
        {
            other_data.push_back(std::make_pair(name, data));
        }
        
        void PutInto(OpcodeTree& tree, unsigned bytepos=0)
        {
            /* Encodes the data into the opcode tree
             * for optimized retrieval */
        
            // Find out which of the options defines the range
            // for this byte
            bool found=false;
            for(unsigned a=0; a<pos_data.size(); ++a)
            {
                const ElemData& d = pos_data[a].second;
                if(d.KnowRange(bytepos))
                {
                    range<unsigned char> r(d.GetMin(bytepos), d.GetMax(bytepos));
                    
                    OpcodeTree::maptype::iterator i = tree.data.find(r);
                    if(i == tree.data.end())
                    {
                        OpcodeTree subtree;
                        PutInto(subtree, bytepos+1);
                        tree.data.insert(std::make_pair(r, subtree));
                    }
                    else
                    {
                        OpcodeTree& subtree = i->second;
                        PutInto(subtree, bytepos+1);
                    }
                    found=true;
                }
            }
            // If there were no specialisations, use the current node.
            if(!found) tree.choices.push_back(*this);
        }
        
        const EventCode::DecodeResult
        Scan
            (unsigned offset, const unsigned char* data, unsigned length,
             EventCode::DecodingState& state) const
        {
            parammap params;
            
            EventCode::DecodeResult result;
            
            unsigned nbytes = 1;
            for(unsigned a=0; a<pos_data.size(); ++a)
            {
                /* Even if it doesn't have a name, it needs to be decoded
                 * to get the opcode length properly.
                 */
                const char* name = pos_data[a].first;
                const ElemData& elem = pos_data[a].second;
                
                ElemData::FormatResult
                    tmp = elem.Format(offset, data, length, state);
                
                if(name)
                {
                    params[name] = tmp.text;
                }
                result.goto_type   = tmp.goto_type;
                result.goto_target = tmp.goto_target;
                if(tmp.maxoffs > nbytes) nbytes = tmp.maxoffs;
            }
            for(unsigned a=0; a<other_data.size(); ++a)
            {
                const char* name = other_data[a].first;
                const ElemData& elem = other_data[a].second;
                
                if(!name)
                {
                    fprintf(stderr, "No name on 'other_data'?\n");
                    throw false;
                }
                params[name] = elem.Format();
            }
            
            result.code   = FormatString(format, params);
            result.nbytes = nbytes; 
            return result;
        }
        
    private:
        const char* format;
        std::vector<std::pair<const char*, ElemData> > pos_data;
        std::vector<std::pair<const char*, ElemData> > other_data;
    };
    
private:
    class OpcodeTree
    {
    public:
        OpcodeTree() { }
    
        typedef simple_rangemap<unsigned char, OpcodeTree> maptype;
        maptype data;
        std::vector<Command> choices;
        
        void Dump(unsigned indent=0) const
        {
            if(!choices.empty())
            {
                std::fprintf(stderr, "%*s", indent, "");
                std::fprintf(stderr, "%u choices.\n", choices.size());
            }
            for(maptype::const_iterator
                i = data.begin();
                i != data.end();
                ++i)
            {
                int lower = i->first.lower, upper = i->first.upper;
                const OpcodeTree& subtree = i->second;
                
                std::fprintf(stderr, "%*s", indent, "");
                std::fprintf(stderr, "subtree %02X-%02X:\n", lower,upper);
                subtree.Dump(indent+2);
            }
        }
    };
    
private:
    class Initialize
    {
    private:
        struct NamedElem: public std::pair<const char*, ElemData>
        {
        public:
            NamedElem(const char*s, const ElemData& e)
              : std::pair<const char*,ElemData>(s,e) { }
        
            NamedElem& AnnotatePC()
            {
               // this->second.Annotate(ElemData::anno_pc);
                return *this;
            }
            NamedElem& AnnotateNPC()
            {
               // this->second.Annotate(ElemData::anno_npc);
                return *this;
            }
            NamedElem& AnnotateEnemy()
            {
               // this->second.Annotate(ElemData::anno_enemy);
                return *this;
            }
            NamedElem& AnnotateSong()
            {
               // this->second.Annotate(ElemData::anno_song);
                return *this;
            }
            NamedElem& AnnotateSFX()
            {
               // this->second.Annotate(ElemData::anno_sfx);
                return *this;
            }
            NamedElem& SetHighbit()
            {
                this->second.DeclareHighbit();
                return *this;
            }
        };
    
        /* formatting */
        Initialize& operator<< (const char* format)
        {
            Flush();
            cur_command = new Command(format);
            return *this;
        }

        /* set byteposition */
        Initialize& operator<< (int bytepos)
        {
            curpos = bytepos;
            return *this;
        }
        
        /* opcode settings */
        Initialize& operator>> (const NamedElem& data)
        {
            if(cur_command) cur_command->Add(data.second, curpos, data.first);
            return *this;
        }
        /* unnamed elements */
        Initialize& operator>> (const ElemData& data)
        {
            if(cur_command) cur_command->Add(data, curpos);
            return *this;
        }
        Initialize& operator>> (unsigned char ch)
        {
            return *this >> ElemData(1,ch,ch,0,0);
        }

        /* settings */
        Initialize& operator<< (const NamedElem& data)
        {
            if(cur_command) cur_command->Add(data.second, data.first);
            return *this;
        }
        
        /* opcode settings */
        static NamedElem DeclareByte(const char* name, unsigned min=0x00, unsigned max=0xFF,
                                     bool highbittrick=false)
        {
            ElemData tmp(1,min,max,0,0);
            if(highbittrick) tmp.DeclareHighbit();
            return NamedElem(name, tmp);
        }
        static NamedElem DeclareObjectNo(const char* name)
        {
            ElemData tmp(1,0x00,0x7E,0,-1);
            return NamedElem(name, tmp);
        }
        static NamedElem DeclareWord(const char* name, unsigned min=0x0000, unsigned max=0xFFFF)
        {
            return NamedElem(name, ElemData(2,min,max,0,0));
        }
        static NamedElem DeclareLong(const char* name, unsigned min=0x000000, unsigned max=0xFFFFFF)
        {
            return NamedElem(name, ElemData(3,min,max,0,0));
        }
        static NamedElem DeclareNibbleHi(const char* name)
        {
            ElemData result(1, 0x00,0xFF, 0,0);
            result.SetType(ElemData::t_nibble_hi);
            return NamedElem(name, result);
        }
        static NamedElem DeclareNibbleLo(const char* name)
        {
            ElemData result(1, 0x00,0xFF, 0,0);
            result.SetType(ElemData::t_nibble_lo);
            return NamedElem(name, result);
        }
        static NamedElem DeclareOrBitnum(const char* name)
        {
            ElemData result(1, 0x00,0x07, 0,0);
            result.SetType(ElemData::t_orbit);
            return NamedElem(name, result);
        }
        static NamedElem DeclareAndBitnum(const char* name)
        {
            ElemData result(1, 0x00,0x07, 0,0);
            result.SetType(ElemData::t_andbit);
            return NamedElem(name, result);
        }
        static NamedElem Declare7E0000_B(const char* name)
            { return NamedElem(name, ElemData(1,0x00,0xFF,0x7E0000,0)); }
        static NamedElem Declare7E0197_B(const char* name)
            { return NamedElem(name, ElemData(1,0x00,0xFF,0x7E0197,0)); }
        static NamedElem Declare7E0100_B(const char* name)
            { return NamedElem(name, ElemData(1,0x00,0xFF,0x7E0100,0)); }
        static NamedElem Declare7F0000_B(const char* name)
            { return NamedElem(name, ElemData(1,0x00,0xFF,0x7F0000,0)); }
        static NamedElem Declare7F0000_W(const char* name)
            { return NamedElem(name, ElemData(2,0x00,0xFFFF,0x7F0000,0)); }
        static NamedElem Declare7F0200_2(const char* name)
            { return NamedElem(name, ElemData(1,0x00,0xFF,0x7F0200,1)); }
        static NamedElem DeclareOperator(const char* name)
        {
            ElemData result(1, 0x00,0x7F, 0,0);
            result.SetType(ElemData::t_operator);
            return NamedElem(name, result);
        }
        static NamedElem DeclareBlob(const char* name)
        {
            ElemData result(2, 0x0000,0xFFFF, 0,0);
            result.SetType(ElemData::t_blob);
            return NamedElem(name, result);
        }
        static NamedElem DeclareTextBlob(const char* name)
        {
            ElemData result(2, 0x0000,0xFFFF, 0,0);
            result.SetType(ElemData::t_textblob);
            return NamedElem(name, result);
        }
        static NamedElem DeclareDialogBegin(const char* name)
        {
            ElemData result(3, 0x000000,0xFFFFFF, 0,0);
            result.SetType(ElemData::t_dialogbegin);
            return NamedElem(name, result);
        }
        static NamedElem DeclareDialogAddr(const char* name)
        {
            ElemData result(1, 0x00,0xFF, 0,0);
            result.SetType(ElemData::t_dialogaddr);
            return NamedElem(name, result);
        }
        
        /* settings */
        static NamedElem DeclareConst(const char* name, unsigned value)
        {
            if(value <= 0xFF)
                return NamedElem(name, ElemData(1,value,value,value,0));
            if(value <= 0xFFFF)
                return NamedElem(name, ElemData(2,value,value,value,0));
            if(value <= 0xFFFFFF)
                return NamedElem(name, ElemData(3,value,value,value,0));
            return NamedElem(name, ElemData(4,value,value,value,0));
        }

        static NamedElem DeclareProp(const char* name, unsigned address)
        {
            if(address <=0x0000FF) address += 0x7E0100; // D page.
            if(address < 0x7E0000) address += 0x7E0000;
            return DeclareConst(name, address);
        }
        static ElemData DeclareElse()
        {
            // forward goto
            ElemData result(1, 0x00,0xFF, 0,0);
            return result.SetType(ElemData::t_else);
        }
        static ElemData DeclareLoop()
        {
            // backward goto
            ElemData result(1, 0x00,0xFF, 0,0);
            return result.SetType(ElemData::t_loop);
        }
        static ElemData DeclareIf()
        {
            // conditional goto
            ElemData result(1, 0x00,0xFF, 0,0);
            return result.SetType(ElemData::t_if);
        }
        
        void Init()
        {
        *this
<< "[Return]"
    << 0 >> 0x00
    /*
        A = $1C00 of cur obj
        If A == 7
        {
          // There are no more threads.
          return and loop
        }
        X = (A << 7) + (obj number)
        $7F0580[X] = word(0) // Mark the current thread as completed
        <begin>
         $1C00 of cur obj += 1
         X += #$80
         A = $7F0580[X]
        <while A==0>
        returns X as A (execution position)
        return
        
      Returns to the function of lower priority
      (bigger number)
    */

<< "[Execute:%2 [priority:%1] [for:%0] [wait:nothing]]"
    << 0 >> 0x02
    << 1 >> DeclareObjectNo("0") // object number whose execution to alter
    << 2 >> DeclareNibbleHi("1")
    << 2 >> DeclareNibbleLo("2")
    /*
        If ( $1C01 for given obj != #0
        Or   $1100 for given obj & #$80
        Or   $1000 for given obj & #$80 )
        {
          return
        }
        $D9 = $1C00 for given obj // Current thread
        $DF = hi nibble
        If $DF == $D9
        {
          // Already executing the selected thread
          return
        }
        If $DF >= $D9
        {
          // Verify that the thread is undefined
          $E1 = ($DF << 7) + param1
          A = $7F0580[$E1]
          If(!zero)
          {
            // Already defined.
            return
          }
          // Define it.
          $E3 = (param0 << 4) + (lo nibble)*2
          $7F0580[$E1] = given actor's pointer from $7F2001[$E3]
        }
        else
        {
          // Returning to a previous thread?
          
          // Save current code position
          $E1 = ($D9 << 7) + param1
          $7F0580[$E1] = $1180 for given obj
          
          // Load new position
          $E3 = (param0 << 4) + (lo nibble)*2
          $1180 for given obj = given actor's pointer from $7F2001[$E3]
          $1C00 for given obj = $DF
          $1A80 for given obj = 0 (moving flag)
          $1A01 for given obj = 0 (length of movement)
        }
        return
        
        Analysis:
          This function meddles with ANOTHER object.
          It does not alter the execution of SELF.

          If the current thread < %1
            Saves current location to current thread
            Calls function %2 as thread %1
          If the current thread >= %1
            Sets thread %1 exit address from function %2
            Only if not defined yet.
          
          Analysis continued, the thread number
          may represent "priority".

          When the thread number is smaller than
          current, the function is called immediately.
          When the number is bigger, it will be called
          when the current one finishes.

          There are eight priority levels:
             0 1 2 3 4 5 6 7
          The opcode 00 returns to the next priority level,
          like from 6 to 7. If the current is 7 when
          opcode 00 is called, it will loop.

     */
     // Note: used by op 05.

<< "[Execute:%2 [priority:%1] [for:%0] [wait:call]]"
    << 0 >> 0x03
    << 1 >> DeclareObjectNo("0")
    << 2 >> DeclareNibbleHi("1")
    << 2 >> DeclareNibbleLo("2")
    
    /* Same as op 02, except that if the given
       object is currently performing a more urgent
       task, this op will wait until it's completed
       and then transfer it to the new one.
    */
    /*
        if ( $1C01 for given obj != #0 )
          return, loop
        
        If ( $1100 for given obj & #$80
        Or   $1000 for given obj & #$80 )
        {
          return
        }
        
        $D9 = $1C00 for given obj // Current thread
        $DF = hi nibble
        If $DF >= $D9
        {
          return, loop
        }
        
        // Save current code position
        $E1 = ($D9 << 7) + param1
        $7F0580[$E1] = $1180 for given obj
        
        // Load new position
        $E3 = (param1 << 4) + (lo nibble)*2
        $1180 for given obj = given actor's pointer from $7F2001[$E3]
        $1C00 for given obj = $DF
        $1A80 for given obj = 0 (moving flag)
        $1A01 for given obj = 0 (length of movement)
        return
     */
     // Note: used by op 06.

<< "[Execute:%2 [priority:%1] [for:%0] [wait:return]]"
    << 0 >> 0x04
    << 1 >> DeclareObjectNo("0")
    << 2 >> DeclareNibbleHi("1")
    << 2 >> DeclareNibbleLo("2")
    
    /*
       Same as op 03, except that it will wait
       until the target object returns from the
       function.
     */
    /*
       $DB = param0 // object number
       
       $D9 = $1C00 for given obj // Current thread
       $DD = byte2 (param1 and param2)
       $DF = param1
       
       A = $7F0980 for _current_ object
       If(A != 0)
       {
         If ( $1100 for given obj & #$80
         Or   $1000 for given obj & #$80 )
         {
           $7F0980 for _current_ object = 0
           return
         }
         
         If $DF >= $D9:
           return, loop
         
         $7F0980 for _current_ obj = 0
         return
       }
       
       If ( $1C01 for given obj != 0):
         return, loop
       
       If ( $1100 for given obj & #$80
       Or   $1000 for given obj & #$80 )
       {
         return
       }
       
       If $DF >= $D9:
         return, loop
       
       // Save current code position
       $E1 = ($D9 << 7) + param0
       $7F0580[$E1] = $1180 for given obj
       
       // Load new position
       $E3 = (param0 << 4) + (param2)*2
       $1180 for given obj = given actor's pointer from $7F2001[$E3]
       $1C00 for given obj = $DF
       $1A80 for given obj = 0 (moving flag)
       $1A01 for given obj = 0 (length of movement)

       $7F0980 for _current_ obj = 1
       return, loop
    */
     // Note: used by op 07.

<< "[Execute:%2 [priority:%1] [for:%0] [wait:nothing]]"
    << 0 >> 0x05
    << 1 >> Declare7E0197_B("0")//target obj
    << 2 >> DeclareNibbleHi("1")
    << 2 >> DeclareNibbleLo("2")
        // Same as 0x02, but uses object number from table.

<< "[Execute:%2 [priority:%1] [for:%0] [wait:call]]"
    << 0 >> 0x06
    << 1 >> Declare7E0197_B("0")//target obj
    << 2 >> DeclareNibbleHi("1")
    << 2 >> DeclareNibbleLo("2")
    // Same as 0x03, but uses object number from table.

<< "[Execute:%2 [priority:%1] [for:%0] [wait:return]]"
    << 0 >> 0x07
    << 1 >> Declare7E0197_B("0")//target obj
    << 2 >> DeclareNibbleHi("1")
    << 2 >> DeclareNibbleLo("2")
    // Same as 0x04, but uses object number from table.

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x08
    << DeclareProp("0", 0x1C01)
    << DeclareConst("1", 1)

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x09
    << DeclareProp("0", 0x1C01)
    << DeclareConst("1", 0)

<< "[ObjectRemove [for:%0]]"
    << 0 >> 0x0A
    << 1 >> DeclareObjectNo("0")
    // For given obj,
    //   Sets 1100=$80 (code execution: dead)
    //   And  1A81=$00 (drawing mode: none)

<< "[ObjectOrB:%0:%1 [for:%2]]"
    << 0 >> 0x0B
    << 1 >> DeclareObjectNo("2")
    << DeclareProp("0", 0x1000)
    << DeclareConst("1", 0x80)

<< "[ObjectAndB:%0:%1 [for:%2]]"
    << 0 >> 0x0C
    << 1 >> DeclareObjectNo("2")
    << DeclareProp("0", 0x1000)
    << DeclareConst("1", 0x7F)

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x0D
    << 1 >> DeclareByte("1")
    << DeclareProp("0", 0x1C80)

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x0E
    << 1 >> DeclareByte("1")
    << DeclareProp("0", 0x1C81)

<< "[ObjectSetFacing:%0]"
    << 0 >> 0x0F
    << DeclareConst("0", 0) //up
    // puts 1600=0, 1601=0

<< ""
    << 0 >> 0x10
    << 1 >> DeclareElse()

<< ""
    << 0 >> 0x11
    << 1 >> DeclareLoop()

<< "B:%addr %op %value"
    << 0 >> 0x12
    << 1 >> Declare7F0200_2("addr")
    << 2 >> DeclareByte("value")
    << 3 >> DeclareOperator("op")
    << 4 >> DeclareIf()

<< "W:%addr %op %value"
    << 0 >> 0x13
    << 1 >> Declare7F0200_2("addr")
    << 2 >> DeclareWord("value")
    << 4 >> DeclareOperator("op")
    << 5 >> DeclareIf()

<< "B:%addr1 %op %addr2"
    << 0 >> 0x14
    << 1 >> Declare7F0200_2("addr1")
    << 2 >> Declare7F0200_2("addr2")
    << 3 >> DeclareOperator("op")
    << 4 >> DeclareIf()

<< "W:%addr1 %op %addr2"
    << 0 >> 0x15
    << 1 >> Declare7F0200_2("addr1")
    << 2 >> Declare7F0200_2("addr2")
    << 3 >> DeclareOperator("op")
    << 4 >> DeclareIf()

<< "B:%addr %op %value"
    << 0 >> 0x16
    << 1 >> Declare7E0000_B("addr")
    << 2 >> DeclareByte("value")
    << 3 >> DeclareOperator("op")
    << 4 >> DeclareIf()

<< "B:%addr %op %value"
    << 0 >> 0x16
    << 1 >> Declare7E0100_B("addr")
    << 2 >> DeclareByte("value")
    << 3 >> DeclareOperator("op").SetHighbit()
    << 4 >> DeclareIf()

<< "[ObjectSetFacing:%0]"
    << 0 >> 0x17
    << DeclareConst("0", 1) //down
    // puts 1600=1, 1601=0

<< "B:%addr < %0"
    << 0 >> 0x18
    << 1 >> DeclareByte("0")
    << 2 >> DeclareIf()
    << DeclareConst("addr", 0x7F0000)

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x19
    << 1 >> Declare7F0200_2("1")
    << DeclareProp("0", 0x7F0A80)

<< "ObjectB:%0 == %1"
    << 0 >> 0x1A
    << 1 >> DeclareByte("1")
    << 2 >> DeclareIf()
    << DeclareProp("0", 0x7F0A80)

<< "[ObjectSetFacing:%0]"
    << 0 >> 0x1B
    << DeclareConst("0", 2) //left
    // puts 1600=2, 1601=0

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x1C
    << 1 >> Declare7F0000_B("1")
    << DeclareProp("0", 0x7F0A80)

<< "[ObjectSetFacing:%0]"
    << 0 >> 0x1D
    << DeclareConst("0", 3) //right
    // puts 1600=3, 1601=0

<< "[ObjectSetFacing:%0 [for:%1]]"
    << 0 >> 0x1E
    << 1 >> DeclareObjectNo("1")
    << DeclareConst("0", 0) //up
    // puts 1600=0, 1601=0

<< "[ObjectSetFacing:%0 [for:%1]]"
    << 0 >> 0x1F
    << 1 >> DeclareObjectNo("1")
    << DeclareConst("0", 1) //down
    // puts 1600=1, 1601=0

<< "[LetB:%0:%1]"
    << 0 >> 0x20
    << 1 >> Declare7F0200_2("0")
    << DeclareConst("1", 0x7E2980) // member1

<< "[ObjectGetPosition:%1:%2 [for:%0]]"
    << 0 >> 0x21
    << 1 >> DeclareObjectNo("0")
    << 2 >> Declare7F0200_2("1")
    << 3 >> Declare7F0200_2("2")
    /*
       param1 = $1801 for given obj (xcoord)
       param2 = $1881 for given obj (ycoord)
     */

<< "[ObjectGetPosition:%1:%2 [for:%0]]"
    << 0 >> 0x22
    << 1 >> Declare7E0197_B("0")
    << 2 >> Declare7F0200_2("1")
    << 3 >> Declare7F0200_2("2")
    /* Same as op 21, but gets object from $97 instead. */

<< "[ObjectGetFacing:%1 [for:%0]]"
    << 0 >> 0x23
    << 1 >> DeclareObjectNo("0")
    << 2 >> Declare7F0200_2("1")
    /*
       assigns $1600 of given obj to param1.
    */

<< "[ObjectGetFacing:%1 [for:%0]]"
    << 0 >> 0x24
    << 1 >> Declare7E0197_B("0")
    << 2 >> Declare7F0200_2("1")
    /* Same as op 23, but gets object from $97 instead. */

<< "[ObjectSetFacing:%0 [for:%1]]"
    << 0 >> 0x25
    << 1 >> DeclareObjectNo("1")
    << DeclareConst("0", 2) //left
    // puts 1600=2, 1601=0

<< "[ObjectSetFacing:%0 [for:%1]]"
    << 0 >> 0x26
    << 1 >> DeclareObjectNo("1")
    << DeclareConst("0", 3) //right
    // puts 1600=3, 1601=0

<< "ObjectB:%0 == %1 [for:%2]"
    << 0 >> 0x27
    << 1 >> DeclareObjectNo("2")
    << 2 >> DeclareIf()
    << DeclareConst("1", 0)
    << DeclareProp("0", 0x0F00)

<< "ObjectNearUnknown [for:%0]"
    << 0 >> 0x28
    << 1 >> DeclareObjectNo("0")
    << 2 >> DeclareIf()
    /*
        $DB = $1D0A >> 1
        $DD = $1D0E >> 1
        x = $1801 for given obj
        y = $1881 for given obj
        if (x - $DB) in (0, 1, >= 14)
        or (y - $DD) in (0, 1, >= 13)
        {
          @ 668F
          goto.
        }
        return
        
        <evilpeer> Seems to be related to the moving
        Turrents on the Blackbird's Left Wing.
     */

<< "[EndingText:%0]"
    << 0 >> 0x29
    << 1 >> DeclareByte("0")

<< "[OrB:%0:%1]"
    << 0 >> 0x2A
    << DeclareConst("0", 0x7E0154)
    << DeclareConst("1", 4)

<< "[OrB:%0:%1]"
    << 0 >> 0x2B
    << DeclareConst("0", 0x7E0154)
    << DeclareConst("1", 8)

<< "[Unknown2C:%0:%1]"
    << 0 >> 0x2C
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    /*
      $1D3F = 0
      $1D40 = 0
      $1D92 = 0
      $1D8F = word(param1*8)
      $1D91 = word(param2*8)
      purpose unknown
    */

<< "W:%0 <> 00"
    << 0 >> 0x2D
    << 1 >> DeclareIf()
    << DeclareConst("0", 0x7E01F8)

<< "[PaletteSet:%2:%3 [palette:%1]]"
    << 0 >> 0x2E
    << 1 >> DeclareByte("", 0x80, 0x8F) // the actual value matters not.
    << 2 >> DeclareNibbleHi("1") // palette number
    << 2 >> DeclareNibbleLo("2") // starting colour
    << 3 >> DeclareBlob("3")
    // writes to 7E2200-> and 7E2000->
    //
    //      7E2200 is the 512-byte buffer of palettes.
    //      DMA 7 writes it to PPU port 2122 (CGRAM) all the time.

<< "[GFXSetup:%0:%1:%2:%3:%4]"
    << 0 >> 0x2E
    << 1 >> DeclareByte("0", 0x40, 0x5F) // this value matters
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    << 5 >> DeclareByte("4")
    /*
      calls function 4B2C.
         if carry set, returns
         if carry clear:
           $0520[Y] = param0 // 0x40..0x5F.
           $0521[Y] = param1
           $0522[Y] = param2
           $0523[Y] = 8
           $0524[Y] = 0
           $0525[Y] = param4
           $0526[Y] = param0
           $0527[Y] = ((param3 & 0xF0) >> 4) | (param3 & 0xF0)
           $0528[Y] = ((param3 & 0x0F) << 4) | (param3 & 0x0F)

        @4B2C: This function is referred often.
          
          Y = 0
          loop:
            A = $0520[Y]
            if(zero)
              return carry-clear
            Y += #$0C
          :loop while Y<60
          return carry-set
        
          I guess this function allocates a new sprite
          slot for the particular object.

     */

<< "[LetW:%0:%1]"
    << 0 >> 0x2F
    << 1 >> DeclareWord("1")
    << DeclareConst("0", 0x0BE3)

<< "B:%0 & %1"
    << 0 >> 0x30
    << 1 >> DeclareIf()
    << DeclareConst("0", 0x7E01F8)
    << DeclareConst("1", 0x02)

<< "B:%0 & %1"
    << 0 >> 0x31
    << 1 >> DeclareIf()
    << DeclareConst("0", 0x7E01F8)
    << DeclareConst("1", 0x80)

<< "[OrB:%0:%1]"
    << 0 >> 0x32
    << DeclareConst("0", 0x7E0154)
    << DeclareConst("1", 0x10)

<< "[ObjectChangePalette:%0]"
    << 0 >> 0x33
    << 1 >> DeclareByte("0")

<< "ButtonStatus:A"
    << 0 >> 0x34
    << 1 >> DeclareIf()
    // accesses $F2
    // Checks for current status of button?
    // mask $80

<< "ButtonStatus:B"
    << 0 >> 0x35
    << 1 >> DeclareIf()
    // mask $08

<< "ButtonStatus:X"
    << 0 >> 0x36
    << 1 >> DeclareIf()
    // mask $40

<< "ButtonStatus:Y"
    << 0 >> 0x37
    << 1 >> DeclareIf()
    // mask $04

<< "ButtonStatus:L"
    << 0 >> 0x38
    << 1 >> DeclareIf()
    // mask $20

<< "ButtonStatus:R"
    << 0 >> 0x39
    << 1 >> DeclareIf()
    // accesses $F2
    // mask $10

<< "Unknown3B"
    << 0 >> 0x3B
    << 1 >> DeclareIf()
    // does something for $50, tests for bit #$02

<< "Unknown3C"
    << 0 >> 0x3C
    << 1 >> DeclareIf()
    // does something for $50, tests for bit #$80


// button masks:
//    $50:
//      02=left?  80=up?
//    $51:
//      01=start, 02=select, 04=y, 08=b
//      10=r,     20=l,      40=x, 80=a

<< "ButtonPressed:A"
    << 0 >> 0x3F
    << 1 >> DeclareIf()
    // accesses $51
    // Checks if the button has been pressed?
    // mask $80 

<< "ButtonPressed:B"
    << 0 >> 0x40
    << 1 >> DeclareIf()
    // mask $08

<< "ButtonPressed:X"
    << 0 >> 0x41
    << 1 >> DeclareIf()
    // mask $40

<< "ButtonPressed:Y"
    << 0 >> 0x42
    << 1 >> DeclareIf()
    // mask $04

<< "ButtonPressed:L"
    << 0 >> 0x43
    << 1 >> DeclareIf()
    // mask $20

<< "ButtonPressed:R"
    << 0 >> 0x44
    << 1 >> DeclareIf()
    // accesses $F1
    // mask $10

<< "[LetB:%0:%1]"
    << 0 >> 0x47
    << 1 >> DeclareByte("1")
    << DeclareConst("0", 0x7E016B)

<< "[LetB:%addr:%long]"
    << 0 >> 0x48
    << 1 >> DeclareLong("long")
    << 4 >> Declare7F0200_2("addr")

<< "[LetW:%addr:%long]"
    << 0 >> 0x49
    << 1 >> DeclareLong("long")
    << 4 >> Declare7F0200_2("addr")

<< "[LetB:%long:%byte]"
    << 0 >> 0x4A
    << 1 >> DeclareLong("long")
    << 4 >> DeclareByte("byte")

<< "[LetW:%long:%word]"
    << 0 >> 0x4B
    << 1 >> DeclareLong("long")
    << 4 >> DeclareWord("word")

<< "[LetB:%long:%addr]"
    << 0 >> 0x4C
    << 1 >> DeclareLong("long")
    << 4 >> Declare7F0200_2("addr")

<< "[LetW:%long:%addr]"
    << 0 >> 0x4D
    << 1 >> DeclareLong("long")
    << 4 >> Declare7F0200_2("addr")

<< "[StringStore:%long:%data]"
    << 0 >> 0x4E
    << 1 >> DeclareLong("long", 0x7E2C23) // show the character name table in plaintext.
    << 4 >> DeclareTextBlob("data")

<< "[StringStore:%long:%data]"
    << 0 >> 0x4E
    << 1 >> DeclareLong("long", 0x7E0000, 0x7FFFFF)
    << 4 >> DeclareBlob("data")

<< "[LetB:%addr:%byte]"
    << 0 >> 0x4F
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[LetW:%addr:%word]"
    << 0 >> 0x50
    << 1 >> DeclareWord("word")
    << 3 >> Declare7F0200_2("addr")

<< "[LetB:%1:%0]"
    << 0 >> 0x51
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")

<< "[LetW:%1:%0]"
    << 0 >> 0x52
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")

<< "[LetB:%1:%0]"
    << 0 >> 0x53
    << 1 >> Declare7F0000_W("0")
    << 3 >> Declare7F0200_2("1")

<< "[LetW:%1:%0]"
    << 0 >> 0x54
    << 1 >> Declare7F0000_W("0")
    << 3 >> Declare7F0200_2("1")

<< "[LetB:%0:%1]]"
    << 0 >> 0x55
    << 1 >> Declare7F0200_2("1")
    << DeclareConst("0", 0x7F0000)
    // "story line counter"?

<< "[LetB:%addr:%byte]"
    << 0 >> 0x56
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0000_W("addr")

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x57
    << DeclareConst("0", 0).AnnotatePC() // crono
    // firsts checks for a party member.
    // if not a party member:
    //  puts $1100,X <- #$80 object is now dead
    //  puts $1101,X <- param0
    //  returns
    // if member 1: puts $1100,X <- #$00
    // if member 2: puts $1100,X <- #$01
    // if member 3: puts $1100,X <- #$02
    // puts $1C80,X <- #$00 movement props
    // puts $1A81,X <- #$01 drawing mode
    // puts $1B01,X <- #$01 solid
    // puts $1101,X <- param0
    // then continues in a complex way.
    
<< "[LetB:%1:%0]"
    << 0 >> 0x58
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0000_W("1")

<< "[LetW:%1:%0]"
    << 0 >> 0x59
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0000_W("1")

<< "[LetW:%1:%0]"
    << 0 >> 0x5A
    << 1 >> DeclareByte("0")
    << DeclareConst("1", 0x7F0000)
    // "story line counter"

<< "[AddB:%addr:%byte]"
    << 0 >> 0x5B
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x5C
    << DeclareConst("0", 1).AnnotatePC() // marle
    // @41EC
    //     $8E <- objno
    //     A   <- 1
    //     goto 421A

<< "[AddB:%1:%0]"
    << 0 >> 0x5D
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")

<< "[AddW:%1:%0]"
    << 0 >> 0x5E
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")

<< "[SubB:%addr:%byte]"
    << 0 >> 0x5F
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[SubW:%addr:%word]"
    << 0 >> 0x60
    << 1 >> DeclareWord("word")
    << 3 >> Declare7F0200_2("addr")

<< "[SubB:%1:%0]"
    << 0 >> 0x61
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x62
    << DeclareConst("0", 2).AnnotatePC() // lucca

<< "[OrB:%addr:%bit]"
    << 0 >> 0x63
    << 1 >> DeclareOrBitnum("bit")
    << 2 >> Declare7F0200_2("addr")
    // addr |= $FF20[byte]

<< "[OrB:%addr:%bit]"
    << 0 >> 0x63
    << 1 >> DeclareOrBitnum("bit")
    << 2 >> Declare7F0200_2("addr")
    // addr |= $FF20[byte]

<< "[AndB:%addr:%bit]"
    << 0 >> 0x64
    << 1 >> DeclareAndBitnum("bit")
    << 2 >> Declare7F0200_2("addr")
    // addr &= $FF28[byte]

<< "[OrB:%addr:%bit]"
    << 0 >> 0x65
    << 1 >> DeclareOrBitnum("bit")
    << 2 >> Declare7E0000_B("addr")
    // addr_value |= (byte & 0x80) << 1
    // addr |= $FF20[byte & 0x0F]
    
<< "[OrB:%addr:%bit]"
    << 0 >> 0x65
    << 1 >> DeclareOrBitnum("bit").SetHighbit()
    << 2 >> Declare7E0100_B("addr")

<< "[AndB:%addr:%bit]"
    << 0 >> 0x66
    << 1 >> DeclareAndBitnum("bit")
    << 2 >> Declare7E0000_B("addr")

<< "[AndB:%addr:%bit]"
    << 0 >> 0x66
    << 1 >> DeclareAndBitnum("bit").SetHighbit()
    << 2 >> Declare7E0100_B("addr")

<< "[AndB:%addr:%byte]"
    << 0 >> 0x67
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x68
    << DeclareConst("0", 3).AnnotatePC() // frog

<< "[OrB:%addr:%byte]"
    << 0 >> 0x69
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x6A
    << DeclareConst("0", 4).AnnotatePC() // robo

<< "[XorB:%addr:%byte]"
    << 0 >> 0x6B
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x6C
    << DeclareConst("0", 5).AnnotatePC() // ayla

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x6D
    << DeclareConst("0", 6).AnnotatePC() // magus

<< "[ShrB:%addr:%byte]"
    << 0 >> 0x6F
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[IncB:%0]"
    << 0 >> 0x71
    << 1 >> Declare7F0200_2("0")

<< "[IncW:%0]"
    << 0 >> 0x72
    << 1 >> Declare7F0200_2("0")

<< "[DecB:%0]"
    << 0 >> 0x73
    << 1 >> Declare7F0200_2("0")

<< "[LetB:%0:1]"
    << 0 >> 0x75
    << 1 >> Declare7F0200_2("0")

<< "[LetW:%0:1]"
    << 0 >> 0x76
    << 1 >> Declare7F0200_2("0")

<< "[LetB:%0:0]"
    << 0 >> 0x77
    << 1 >> Declare7F0200_2("0")

<< "[ObjectJump:%0:%1:%2]"
    << 0 >> 0x7A
    << 1 >> DeclareByte("0") //x
    << 2 >> DeclareByte("1") //y
    << 3 >> DeclareByte("2") //height
    // geometrically "jump"

<< "[ObjectPerformMovement:%0:%1:%2:%3]"
    << 0 >> 0x7B
    << 1 >> DeclareByte("0") // $1900 ?
    << 2 >> DeclareByte("1") // $1980 ?
    << 3 >> DeclareByte("2") // $1B81 ?
    << 4 >> DeclareByte("3") // $1A01 length of movement
    /* Waits until 1A01 becomes zero again. */

<< "[ObjectLetB:%2:%3 [for:%0]]"
    << 0 >> 0x7C
    << 1 >> DeclareObjectNo("0")
    << DeclareProp("2", 0x1A81)
    << DeclareConst("3", 1)
    // Sets drawing "on" */

<< "[ObjectLetB:%2:%3 [for:%0]]"
    << 0 >> 0x7D
    << 1 >> DeclareObjectNo("0")
    << DeclareProp("2", 0x1A81)
    << DeclareConst("3", 0)
    // Sets drawing "off" */

<< "[ObjectLetB:%2:%3]"
    << 0 >> 0x7E
    << DeclareProp("2", 0x1A81)
    << DeclareConst("3", 0x80)
    // Sets drawing "hide" */

<< "[GetRandom:%0]"
    << 0 >> 0x7F
    << 1 >> Declare7F0200_2("0")
    // does:
    //  A  = ++$7E01F8
    //  %0 = $FE00[A & 0xFF]

<< "[ObjectLoadPC:%0 [IfInParty]]"
    << 0 >> 0x80
    << 1 >> DeclareByte("0").AnnotatePC()
    // @421E
    // if not a party member:
    //  puts $1100,X <- #$80   (marks object dead)
    //  puts $1101,X <- param
    //  returns
    // if the given value is 0..6,
    //  sets $8D+param = current object number
    // Then follows with the actual thing.
    // This function is called by opcodes
    // 57,5C,62,68,6A,6C and 6D.
    // 

<< "[ObjectLoadPC:%0 [As NPC]]"
    << 0 >> 0x81
    << 1 >> DeclareByte("0").AnnotatePC()
    // @4476
    //    Somehow differs from op 80.
    /*
           $1100 for cur obj <- 3 (object identifier)
           $1B01 for cur obj <- 1
           $1101 for cur obj <- param0
           $BF = param0 * 5
           $A = $E4F001[$BF]
           and so on.
    */

<< "[ObjectLoadNPC:%0]"
    << 0 >> 0x82
    << 1 >> DeclareByte("0").AnnotateNPC()

<< "[ObjectLoadEnemy:%m:%1]"
    << 0 >> 0x83
    << 1 >> DeclareByte("m").AnnotateEnemy()
    << 2 >> DeclareByte("1")
    // the meaning of the second param is unknown

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x84
    << 1 >> DeclareByte("1")
    << DeclareProp("0", 0x1B01)
    // "npc solid props"

<< "[ObjectSet1000:%0]"
    << 0 >> 0x87
    << 1 >> DeclareByte("0")
    // $1000,X  &=  #$80
    // $1000,X  |=  (param+1)
    // $1001,X  = $1000,X

<< "[ObjectPaletteReset]"
    << 0 >> 0x88
    << 1 >> 0x00
    /* 
        @48C0
          
          Y = $7F0B80 for cur obj
          if Y & #$80
            return
          $7F0B80 for cur obj = #$FFFF
          $0520[Y] = #$00
          Y = ($0F81 for cur obj) << 4
          X = ($1400 for cur obj)
          Copy 18 bytes from $E4:X to $7E:(#$2102+Y)
          Copy 18 bytes from $E4:X to $7E:(#$2302+Y)
          return
        
        Seems to load the object's palette from ROM and set it.
          
     */

<< "[ObjectGFXSetup:%0:%1:%2:%3]"
    << 0 >> 0x88
    << 1 >> DeclareByte("0", 0x20, 0x20) // this value matters
    << 2 >> DeclareByte("1")
    << 3 >> DeclareNibbleLo("2")
    << 3 >> DeclareNibbleHi("3")
    /* 
        @4919
        
          call $4B2C
          if carry set:
            return
          $7F0B80 for cur obj = Y
          $0520[Y] = param0 // the 0x20,0x30 byte.
          $0521[Y] = param3 + (($0F81 for cur obj) << 3) + 0x80
          $0522[Y] = param2
          $0524[Y] = 0
          $0525[Y] = param1
          return
          
          
     */

<< "[ObjectGFXSetup:%0:%1:%2:%3]"
    << 0 >> 0x88
    << 1 >> DeclareByte("0", 0x30, 0x30) // this value matters
    << 2 >> DeclareByte("1")
    << 3 >> DeclareNibbleLo("2")
    << 3 >> DeclareNibbleHi("3")
    // same as 88 20

<< "[ObjectGFXSetup:%0:%1:%2:%3:%4]"
    << 0 >> 0x88
    << 1 >> DeclareByte("0", 0x40, 0x5F) // this value matters
    << 2 >> DeclareNibbleLo("1")
    << 2 >> DeclareNibbleHi("2")
    << 3 >> DeclareByte("3")
    << 4 >> DeclareByte("4")
    /* 
        @4970
        
          call $4B2C
          if carry set:
            return
          
          $0520[Y] = param0 & 0xF0 // the 0x40..0x5F byte.
          $0521[Y] = param2 + (($0F81 for cur obj) << 3) + 0x80
          $0522[Y] = param1
          $0523[Y] = 8
          $0524[Y] = 0
          $0525[Y] = param4
          $0526[Y] = param0 & 0x0F
          $0527[Y] = ((param3 & 0xF0) >> 4) | (param3 & 0xF0)
          $0528[Y] = ((param3 & 0x0F) << 4) | (param3 & 0x0F)
          return
          
          
     */

<< "[ObjectPaletteSet:%0:%1]"
    << 0 >> 0x88
    << 1 >> DeclareByte("", 0x80, 0x8F) // actual value matters
    << 1 >> DeclareNibbleLo("0") // starting colour
    << 2 >> DeclareBlob("1")
    /* 
       @49F8
         
          $DB = (($0F81 for cur obj) << 3) + 0x80
          $DD = ((param0 & 0x0F) + $DB) * 2
          writes the blob into $7E2200+[$DB] and $7E2000+[$DB]
          
          7E2200 is the 512-byte buffer of palettes.
          DMA 7 writes it to PPU port 2122 (CGRAM) all the time.
     */

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x89
    << 1 >> DeclareByte("1")
    << DeclareProp("0", 0x1A00)
    // "npc speed"

<< "[ObjectLetB:%0:%1]"
    << 0 >> 0x8A
    << 1 >> Declare7F0200_2("1")
    << DeclareProp("0", 0x1A00)
    // "npc speed"

<< "[ObjectSetCoord:%0:%1]"
    << 0 >> 0x8B
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    // $1800,X = 80xx where xx=%0
    // $1880,X = FFxx where xx=%1

<< "[ObjectSetCoord:%0:%1]"
    << 0 >> 0x8C
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")

<< "[SetObjectCoord2:%0:%1]"
    << 0 >> 0x8D
    << 1 >> DeclareWord("0")
    << 3 >> DeclareWord("1")

<< "[ObjectHide:%0]"
    << 0 >> 0x8E
    << 1 >> DeclareByte("0")
    /*
    > 03 - bottom half
    > 0C - ???
    > 30 - top half
    > 40 - ???
    > 80 - determines mode
    */

<< "[ObjectMoveTowards:%0 [facing:rotate]]"
    << 0 >> 0x8F
    << 1 >> Declare7E0197_B("0")//target obj
    /*
       if ( $1A01 for current object  <> 0 )
         return, loop this command.
       
       if ( $1100 for given object & #$80 )
       {
         // if the object is removed?
         call $568B
         return
       }
       
       @5444
       [$F2] = $1801 for given obj
       [$F3] = $1881 for given obj
       [$F0] = $1801 for current object
       [$F1] = $1881 for current object
       
       if ( abs([$F1] - [$F3]) <= 1
       and  abs([$F0] - [$F2]) <= 1 )
       {
         call $568B
         return
       }
       @547C
       $DB = $1D0A >> 1
       $DD = $1D0E >> 1
       if ($F0 - $DB) in (0, 1, >= 14)
       or ($F1 - $DD) in (0, 1, >= 13)
       {
         @ 54B6
         call $ABA2
         ...
         return, loop
       }
       call $568B
       return
       
       .....
       
       
       "rotate" is probably incorrect, but it's used
       when the code contains a #$30 added to the angle.
      
     */

<< "[ObjectLetB:%2:%3]"
    << 0 >> 0x90
    << DeclareProp("2", 0x1A81)
    << DeclareConst("3", 1)
    // Sets drawing "on" */

<< "[ObjectLetB:%2:%3]"
    << 0 >> 0x91
    << DeclareProp("2", 0x1A81)
    << DeclareConst("3", 0)
    // Sets drawing "off" */

<< "[ObjectMoveAngle:%0:%1 [facing:change]]"
    << 0 >> 0x92
    << 1 >> DeclareByte("0") //angle     ($40 = 90 degrees)
    << 2 >> DeclareByte("1") //magnitude

<< "[ObjectMoveTowards:%0 [facing:change]]"
    << 0 >> 0x94
    << 1 >> DeclareObjectNo("0") //target obj
    // included by op B5
    /*
       if ( $1A01 for current object  <> 0 )
         return, loop this command.
       
       if ( $1100 for given object & #$80 )
       {
         // if the object is removed?
         call $527B
         return
       }
       
       @522D
       [$F2] = $1801 for given obj
       [$F3] = $1881 for given obj
       [$F0] = $1801 for current object
       [$F1] = $1881 for current object
       
       if ( abs([$F1] - [$F3]) > 1
       or   abs([$F0] - [$F2]) > 1 )
       {
         @5281
         call $ABA2 // calculate facing
         .....
       }
       @5265
       if ( $1C81 for current object & #$02 )
       {
         @526E
         call $30B3
         if carry set
         {
           @527B
           call $5614
           return, loop
         }
       }
       call $568B
       return
    */
    
<< "[ObjectMoveTowards:%0 [facing:change]]"
    << 0 >> 0x95
    << 1 >> Declare7E0197_B("0")//target obj
    // included by op B6
    /*
       if ( $1A01 for current object <> 0 )
         return, loop this command.
       
       load the object number of the party leader
       (from $97[param])
       then goto op $94.

       $97 is a word. The object number of first party member?
       Bit $80 means inactive.
       Ha! Indeed.
       $9B is a word too - it refers to an object. (?)
       But it's not used here.
    */
    /* Execution continues (maybe) when the goal has been reached. */

<< "[ObjectMoveTowards:%0:%1 [facing:change]]"
    << 0 >> 0x96
    << 1 >> DeclareByte("0") //xcoord
    << 2 >> DeclareByte("1") //ycoord

<< "[ObjectMoveTowards:%0:%1 [facing:change]]"
    << 0 >> 0x97
    << 1 >> Declare7F0200_2("0") //xcoord from-var
    << 2 >> Declare7F0200_2("1") //ycoord from-var
    /* Same as $96 but coordinates are loaded from vars. */

<< "[ObjectMoveTowardsBy:%0:%1 [facing:change]]"
    << 0 >> 0x98
    << 1 >> DeclareObjectNo("0") // the target obj.
    << 2 >> DeclareByte("1") // length of movement
    /* The same as 0x94, except that 1A80
     * plays some important part here. */
    /*
       if ( $1A80 for current object <> 0)
       {
         if ( $1A01 for current object  <> 0 )
           return, loop this command.
         $1A80 for current object <- 0
         return
       }
       
       if ( $1100 for given object & #$80 )
       { 
         // if the object is removed?
         $1A80 for given object <- 0
         call $568B
         return
       }
       
       @5370
       [$F2] = $1801 for given obj
       [$F3] = $1881 for given obj
       [$F0] = $1801 for current object
       [$F1] = $1881 for current object
       
       if ( abs([$F1] - [$F3]) > 1
       or   abs([$F0] - [$F2]) > 1 )
       {
         @53CA
         call $ABA2
         .....
       }
       @53A8
       if ( $1C81 for current object & #$02 )
       {
         @53B1
         call $30B3
         if carry set
         {
           @53C4
           call $5614
           return, loop
         }
       }
       $1A80 for current object <- 0
       call $568B
       return
    */

<< "[ObjectMoveTowardsBy:%0:%1 [facing:change]]"
    << 0 >> 0x99
    << 1 >> Declare7E0197_B("0") // the target obj.
    << 2 >> DeclareByte("1")     // length of movement

<< "[ObjectMoveTowardsBy:%0:%1:%2 [facing:change]]"
    << 0 >> 0x9A
    << 1 >> DeclareByte("0") // xcoord
    << 2 >> DeclareByte("1") // ycoord
    << 3 >> DeclareByte("2") // length of movement

<< "[ObjectMoveAngle:%0:%1 [facing:keep]]"
    << 0 >> 0x9C
    << 1 >> DeclareByte("0") // angle ($40 = 90 degrees)
    << 2 >> DeclareByte("1") // length of movement
    // Same as op 92, but without changing facing

<< "[ObjectMoveAngle:%0:%1 [facing:keep]]"
    << 0 >> 0x9D
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")
    // Same as op 9C, but from vars

<< "[ObjectMoveTowards:%0 [facing:keep]]"
    << 0 >> 0x9E
    << 1 >> DeclareObjectNo("0") // target obj
    // same as op 94, but without changing facing

<< "[ObjectMoveTowards:%0 [facing:keep]]"
    << 0 >> 0x9F
    << 1 >> Declare7E0197_B("0") // target obj
    // same as op 9E, but from var (such as party member)

<< "[ObjectMoveTowards:%0:%1 [facing:rotate]]"
    << 0 >> 0xA0
    << 1 >> DeclareByte("0") //xcoord
    << 2 >> DeclareByte("1") //ycoord
    /*
       "rotate" is probably incorrect, but it's used
       when the code contains a #$30 added to the angle.
    */

<< "[ObjectMoveTowards:%0:%1 [facing:rotate]]"
    << 0 >> 0xA1
    << 1 >> Declare7F0200_2("0") //xcoord from
    << 2 >> Declare7F0200_2("1") //ycoord from
    /* Used for Robo's following of party members at End of time */
    /*
       "rotate" is probably incorrect, but it's used
       when the code contains a #$30 added to the angle.
    */

<< "[ObjectSetFacing:%0]"
    << 0 >> 0xA6
    << 1 >> DeclareByte("0")

<< "[ObjectSetFacing:%0]"
    << 0 >> 0xA7
    << 1 >> Declare7F0200_2("0")

<< "[ObjectSetFacingTowards:%0]"
    << 0 >> 0xA8
    << 1 >> DeclareObjectNo("0")

<< "[ObjectSetFacingTowards:%0]"
    << 0 >> 0xA9
    << 1 >> Declare7F0200_2("0")

<< "[ObjectAnimation:%0 [mode:1]]"
    << 0 >> 0xAA
    << 1 >> DeclareByte("0")
    /*
         $1680 for cur obj = byte1
         $1780 for cur obj = 1
         $1601 for cur obj = 0
         $1681 for cur obj = 0
       $7F0B01 for cur obj = 0
       
     op B3 is the same with byte1=0
     op B4 is the same with byte1=1
       
     */

<< "[ObjectAnimation:%0 [mode:2]]"
    << 0 >> 0xAB
    << 1 >> DeclareByte("0")
    /*
        A = $7F0B01 of cur obj
        If(A > 0)
        {
          --A
          If(A == 0)
          {
            // end loop.
            $7F0B01 of cur obj = 0
            $1601 of cur obj = 0
            $1681 of cur obj = 0
            if($1680 of cur obj == #$FF)
            {
                $1680 of cur obj = #0
                $1780 of cur obj = 0
            }
            else
            {
                $1780 of cur obj = 1
            }
            return // end loop.
          }
          // wait until the animation frame has changed.
          if(byte1 == $1781 of cur obj)
          {
            return, loop
          }
        }
        $1781 of cur obj = byte1
        $1681 of cur obj = 0
        $1601 of cur obj = 0
        if($1780 of cur obj == 0)
        {
          $1680 of cur obj = #$FF
        }
        $1780 of cur obj = 2
        $7F0B01 of cur obj = 2
        return, loop
    */

<< "[ObjectAnimation:%0 [mode:3]]"
    << 0 >> 0xAC
    << 1 >> DeclareByte("0")
    /*
         $1301 for cur obj = byte1
         $1601 for cur obj = 0
         $1681 for cur obj = 0
         if($1780 for cur obj <> 0)
         {
           $1680 for cur obj = #$FF
         }
         $1780 for cur obj = 3
         return
     */

<< "[Pause:%0]"
    << 0 >> 0xAD
    << 1 >> DeclareByte("0")

<< "[ObjectAnimationReset]"
    << 0 >> 0xAE

<< "[PartyAction [once]]"
    << 0 >> 0xAF

<< "[PartyAction [forever]]"
    << 0 >> 0xB0
    /*
        // referred by op B0
        If $38 <> 0, returns.
        
        A = $1100 for current obj. (party identity)
        switch(A)
        {
          case 0: // member1
            $1A01 for current obj = 1 (movement length)
            call 9E29
            A = $1600 for current obj (facing)
            call 5B8D
            if carry set:
              call 3154
            break;
          case 1: // member2
            $1A01 for current obj = 1
            call A26B
            break;
          case 2: // member3
            $1A01 for current obj = 1
            call A2C2
            break;
        }
        return, loop.
        
        Analysis: This opcode lets the player control the party members.
        If the object is member1, it responds to controls.
        If the object is member2, it imitates member1 with delay.
        If the object is member3, it imitates member2 with delay.
        I did not disassemble those called functions, but I'm quite
        sure for this...
    */

<< "[Yield]"
    << 0 >> 0xB1
    /* This command tells the task manager that this object
     * wants now other objects to execute one cycle of
     * whatever they are doing.
     * It is usually issued in loops, before rerunning the loop.
     */

<< "[Yield [forever]]"
    << 0 >> 0xB2
    /* This function is used when the current function
     * has nothing to do, but for some reason it doesn't
     * want to [Return] to the lower priority routine.
     *
     * It simply will [Yield] forever, without altering
     * the object's state.
     */

<< "[ObjectAnimation:%0 [mode:1]]"
    << 0 >> 0xB3
    << DeclareConst("0", 0)
    // Same as op AA, but with value 0

<< "[ObjectAnimation:%0 [mode:1]]"
    << 0 >> 0xB4
    << DeclareConst("0", 1)
    // Same as op AA, but with value 1

<< "[ObjectMoveTowards:%0 [facing:change] [forever]]"
    << 0 >> 0xB5
    << 1 >> DeclareObjectNo("0")
    // loops op 94 forever

<< "[ObjectMoveTowards:%0 [facing:change] [forever]]"
    << 0 >> 0xB6
    << 1 >> Declare7E0197_B("0")
    // loops op 95 forever

<< "[ObjectAnimation:%0:%1 [mode:2]]"
    << 0 >> 0xB7
    << 1 >> DeclareByte("0") // animation index, possibly
    << 2 >> DeclareByte("1") // duration, possibly
    /*
        A = $7F0B01 of cur obj
        If(A > 0)
        {
          --A
          If(A == 0)
          {
            // end loop.
            $7F0B01 of cur obj = 0
            $1601 of cur obj = 0
            $1681 of cur obj = 0
            if($1600 of cur obj == #$FF)
            {
                $1680 of cur obj = #0
                $1780 of cur obj = 0
            }
            else
            {
                $1780 of cur obj = 1
            }
            return // end loop.
          }
          // wait until the animation frame has changed.
          if(byte1 == $1781 of cur obj)
          {
            return, loop
          }
        }
        $1781 of cur obj = byte1
        $1681 of cur obj = 0
        $1601 of cur obj = 0
        if($1780 of cur obj == 0)
        {
          $1680 of cur obj = #$FF
        }
        $1780 of cur obj = #2
        $7F0B01 of cur obj = byte2 + 1
        return, loop
    */

<< "[DialogSetTable:%0]"
    << 0 >> 0xB8
    << 1 >> DeclareDialogBegin("0")

<< "[Pause:250ms]"
    << 0 >> 0xB9

<< "[Pause:500ms]"
    << 0 >> 0xBA

<< "[DialogDisplay:%0 [pos:auto]]"
    << 0 >> 0xBB
    << 1 >> DeclareDialogAddr("0")

<< "[Pause:1000ms]"
    << 0 >> 0xBC

<< "[Pause:2000ms]"
    << 0 >> 0xBD

<< "[DialogAsk:%0:%1:%2 [pos:auto]]"
    << 0 >> 0xC0
    << 1 >> DeclareDialogAddr("0")
    << 2 >> DeclareByte("2")
    << DeclareProp("1", 0x7F0A80)

<< "[DialogDisplay:%0 [pos:top]]"
    << 0 >> 0xC1
    << 1 >> DeclareDialogAddr("0")

<< "[DialogDisplay:%0 [pos:bottom]]"
    << 0 >> 0xC2
    << 1 >> DeclareDialogAddr("0")

<< "[DialogAsk:%0:%1:%2 [pos:top]]"
    << 0 >> 0xC3
    << 1 >> DeclareDialogAddr("0")
    << 2 >> DeclareByte("2")
    << DeclareProp("1", 0x7F0A80)

<< "[DialogAsk:%0:%1:%2 [pos:bottom]]"
    << 0 >> 0xC4
    << 1 >> DeclareDialogAddr("0")
    << 2 >> DeclareByte("2")
    << DeclareProp("1", 0x7F0A80)

<< "[ItemGive:%0]"
    << 0 >> 0xC7
    << 1 >> Declare7F0200_2("0")
    // adds an item from the given location

<< "[DialogDisplaySpecial:%0]"
    << 0 >> 0xC8
    << 1 >> DeclareByte("0")
    // shops, name entries and such.

<< "HasItem:%0"
    << 0 >> 0xC9
    << 1 >> DeclareByte("0")
    << 2 >> DeclareIf()

<< "[ItemGive:%0]"
    << 0 >> 0xCA
    << 1 >> DeclareByte("0")

<< "[ItemTake:%0]"
    << 0 >> 0xCB
    << 1 >> DeclareByte("0")

<< "HasGold:%0"
    << 0 >> 0xCC
    << 1 >> DeclareWord("0")
    << 3 >> DeclareIf()

<< "[GoldGive:%0]"
    << 0 >> 0xCD
    << 1 >> DeclareWord("0")

<< "[GoldTake:%0]"
    << 0 >> 0xCE
    << 1 >> DeclareWord("0")

<< "HasMember:%0"
    << 0 >> 0xCF
    << 1 >> DeclareByte("0")
    << 2 >> DeclareIf()

<< "[GiveMember:%0]"
    << 0 >> 0xD0
    << 1 >> DeclareByte("0")

<< "[TakeMember:%0]"
    << 0 >> 0xD1
    << 1 >> DeclareByte("0")

<< "HasActiveMember:%0"
    << 0 >> 0xD2
    << 1 >> DeclareByte("0")
    << 2 >> DeclareIf()

<< "[GiveActiveMember:%0]"
    << 0 >> 0xD3
    << 1 >> DeclareByte("0")

<< "[UnactivateMember:%0]"
    << 0 >> 0xD4
    << 1 >> DeclareByte("0")

<< "[EquipMember:%0:%1]"
    << 0 >> 0xD5
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")

<< "[TakeActiveMember:%0]"
    << 0 >> 0xD6
    << 1 >> DeclareByte("0")

<< "[ItemQueryAmount:%byte:%addr]"
    << 0 >> 0xD7
    << 1 >> DeclareByte("byte")
    << 2 >> Declare7F0200_2("addr")

<< "[StartBattle:%0]"
    << 0 >> 0xD8
    << 1 >> DeclareWord("0")

<< "[PartyMove:%x1:%y1:%x2:%y2:%x3:%y3]"
    << 0 >> 0xD9
    << 1 >> DeclareByte("x1")
    << 2 >> DeclareByte("y1")
    << 3 >> DeclareByte("x2")
    << 4 >> DeclareByte("y2")
    << 5 >> DeclareByte("x3")
    << 6 >> DeclareByte("y3")
    /* Moves the party members to the given positions */
    // writes to 1180

<< "[PartySetFollow]"
    << 0 >> 0xDA
    /* causes members 2 and 3 to follow member 1 again. */
    // writes to 1180

<< "[PartyTeleportDC:%0:%1:%2:%3]"
    << 0 >> 0xDC
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    /*
        %1: 7654321076543210: stored to $0C. (x and y?)
        %0: 7654321076543210
                   ^^^^^^^^^: stored to $0A.
            ^  ^^^^         : stored to $0E. (b000zzzz)
     */

<< "[PartyTeleportDD:%0:%1:%2:%3]"
    << 0 >> 0xDD
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    /*
        %1: 7654321076543210: stored to $02. (x and y?)
        %0: 7654321076543210
                   ^^^^^^^^^: stored to $00.
            ^  ^^^^         : stored to $04. (b000zzzz)
    */

<< "[PartyTeleportDD:%0:%1:%2:%3 [with 1E=1]]"
    << 0 >> 0xDE
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    // Same as DD, but puts $1E = 1 */

<< "[PartyTeleportE1:%0:%1:%2:%3 [with 1E=1]]"
    << 0 >> 0xDF
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    // Same as E1, but puts $1E = 1 */

<< "[PartyTeleportE0:%0:%1:%2:%3]"
    << 0 >> 0xE0
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    /*
       Waits until [$17] & 0x80 = 0.

        %1: 7654321076543210: stored to $14 (x and y?)
        %0: 7654321076543210
                   ^^^^^^^^^: stored to $12.
            ^  ^^^^         : stored to $16. (b000zzzz)
        Sets   $17 |= 0x80
        Stores $19 = 0x0F
     */

<< "[PartyTeleportE1:%0:%1:%2:%3]"
    << 0 >> 0xE1
    << 1 >> DeclareByte("0")
    << 2 >> DeclareByte("1")
    << 3 >> DeclareByte("2")
    << 4 >> DeclareByte("3")
    /*
        %1: 7654321076543210: stored to $14. (x and y?)
        %0: 7654321076543210
                   ^^^^^^^^^: stored to $12.
            ^  ^^^^         : stored to $16. (b000zzzz)
        Waits for vrefresh.
        Calls 0B4E.
        Sets $05 = 0x0000
        Sets $07 = 0x0002
        Sets $09 = [$04]
        Sets $00 = [$12]
        Sets $02 = [$14]
        Sets $04 = [$16]
        Then restarts the scene.
     */
    

<< "[PartyTeleportE0:%0:%1:%2:%3]"
    << 0 >> 0xE2
    << 1 >> Declare7F0200_2("0")
    << 2 >> Declare7F0200_2("1")
    << 3 >> Declare7F0200_2("2")
    << 4 >> Declare7F0200_2("3")
    /*
       Same as E0, except the values
       are loaded from given memory locations.
    
     */

<< "[LetB:%1:%0]"
    << 0 >> 0xE3
    << 1 >> DeclareByte("0")
    << DeclareConst("1", 0x7E011F)
           // Explore mode

<< "[CopyTiles:%l:%t:%r:%b:%x:%y:%f [v0]]"
    << 0 >> 0xE4
    << 1 >> DeclareByte("l")
    << 2 >> DeclareByte("t")
    << 3 >> DeclareByte("r")
    << 4 >> DeclareByte("b")
    << 5 >> DeclareByte("x")
    << 6 >> DeclareByte("y")
    << 7 >> DeclareByte("f")
    // Pokes the parameters to $3E,$40,$42,$44 and calls $AF4E.

<< "[CopyTiles:%l:%t:%r:%b:%x:%y:%f [v1]]"
    << 0 >> 0xE5
    << 1 >> DeclareByte("l")
    << 2 >> DeclareByte("t")
    << 3 >> DeclareByte("r")
    << 4 >> DeclareByte("b")
    << 5 >> DeclareByte("x")
    << 6 >> DeclareByte("y")
    << 7 >> DeclareByte("f")
    // Pokes the parameters to $3E,$40,$42,$44
    // then puts (f & 7) to $45 and does [$17] |= 0x20.

<< "[ScrollLayers:%0:%1:%2]"
    << 0 >> 0xE6
    << 1 >> DeclareWord("0")
    << 3 >> DeclareByte("1")
    << 4 >> DeclareByte("2")

<< "[ScrollScreen:%0:%1]"
    << 0 >> 0xE7
    << 1 >> DeclareByte("0") //x
    << 2 >> DeclareByte("1") //y

<< "[PlaySound:%a [pan=%p]]"
    << 0 >> 0xE8
    << 1 >> DeclareByte("a")
    << DeclareConst("p", 0x80)
    /* Command E8 pokes $1E00..$1E02 as 18:a:80
     *  and calls a sound function
     */

<< "[SongPlay:%a]"
    << 0 >> 0xEA
    << 1 >> DeclareByte("a").AnnotateSong()
    /* Command EA pokes $1E00..$1E01 as 10:a
     * and stores a into $7E29AE
     *  and calls a sound function
     */

<< "[SetVolume:%l:%r]"
    << 0 >> 0xEB
    << 1 >> DeclareByte("l")
    << 2 >> DeclareByte("r")
    /* Command EA pokes $1E00..$1E03 as 81:l:r:FF
     *  and calls a sound function
     *       then pokes $1E00..$1E02 as 82:00:FF
     *  and calls a sound function.
     */

/* Command EC pokes $1E00..$1E02 as a:b:c
 * and calls a sound function.
 */

<< "[SongPlay:%a [generic]]"
    << 0 >> 0xEC
    << 1 >> 0x11
    << 2 >> DeclareByte("a").AnnotateSong()
    << 3 >> 0x00

<< "[SongPlay:%a [keep pos]]"
    << 0 >> 0xEC
    << 1 >> 0x14
    << 2 >> DeclareByte("a").AnnotateSong()
    << 3 >> 0x00

<< "[PlaySound:%a [pan=%p]]"
    << 0 >> 0xEC
    << 1 >> DeclareByte("", 0x18, 0x19)
    << 2 >> DeclareByte("a").AnnotateSFX()
    << 3 >> DeclareByte("p")

<< "[SongFade:%volume [duration=%duration]]"
    << 0 >> 0xEC
    << 1 >> 0x82
    << 2 >> DeclareByte("duration")
    << 3 >> DeclareByte("volume")

<< "[SongChangeTempo:%tempo [duration=%duration]]"
    << 0 >> 0xEC
    << 1 >> DeclareByte("", 0x85, 0x86)
    << 2 >> DeclareByte("duration")
    << 3 >> DeclareByte("tempo")

<< "[SongChangeState]"
    << 0 >> 0xEC
    << 1 >> 0x88
    << 2 >> 0x01
    << 3 >> 0x01

<< "[SongMute]"
    << 0 >> 0xEC
    << 1 >> 0xF0
    << 2 >> 0x00
    << 3 >> 0x00

<< "[SoundEffectMute]"
    << 0 >> 0xEC
    << 1 >> 0xF2
    << 2 >> 0x00
    << 3 >> 0x00

<< "[SongChangeState]"
    << 0 >> 0xEC
    << 1 >> 0x88
    << 2 >> 0x01
    << 3 >> 0x01

<< "[SoundCommand:%a:%b:%c]"
    << 0 >> 0xEC
    << 1 >> DeclareByte("a")
    << 2 >> DeclareByte("b")
    << 3 >> DeclareByte("c")

<< "[WaitSilence]"
    << 0 >> 0xED

<< "[WaitSongEnd]"
    << 0 >> 0xEE

<< "[FadeOutScreen:%0]"
    << 0 >> 0xF0
    << 1 >> DeclareByte("0")

<< "[BrightenScreen:%index]"
    << 0 >> 0xF1
    << 1 >> DeclareByte("index", 0x00, 0x00)

<< "[BrightenScreen:%index:%duration]"
    << 0 >> 0xF1
    << 1 >> DeclareByte("index", 0x01, 0xFF)
    << 2 >> DeclareByte("duration")

<< "[FadeOutScreen]"
    << 0 >> 0xF2

<< "[WaitFor18bit08]"
    << 0 >> 0xF3

<< "[ShakeScreen:%0]"
    << 0 >> 0xF4
    << 1 >> DeclareByte("0")

<< "[HealHPandMP]"
    << 0 >> 0xF8
    /* does opF9 and opFA */

<< "[HealHP]"
    << 0 >> 0xF9
    /* calls C28004,A=6 */

<< "[HealMP]"
    << 0 >> 0xFA
    /* calls C28004,A=7 */

<< "[SetGeometry:%a1:%a2:%b1:%b2:%c1:%c2:%d1:%d2:%e1:%e2:%f1:%f2:%g1:%g2:%h1:%h2 [divider=%divider]]"
    << 0 >> 0xFE
    << 1 >> DeclareByte("divider")
    << 2 >> DeclareByte("a1")
    << 3 >> DeclareByte("a2")
    << 4 >> DeclareByte("b1")
    << 5 >> DeclareByte("b2")
    << 6 >> DeclareByte("c1")
    << 7 >> DeclareByte("c2")
    << 8 >> DeclareByte("d1")
    << 9 >> DeclareByte("d2")
    << 10 >> DeclareByte("e1")
    << 11 >> DeclareByte("e2")
    << 12 >> DeclareByte("f1")
    << 13 >> DeclareByte("f2")
    << 14 >> DeclareByte("g1")
    << 15 >> DeclareByte("g2")
    << 16 >> DeclareByte("h1")
    << 17 >> DeclareByte("h2")
    /*
       $F4     = byte1
       $7F1CE8 = byte1
       $C7     = pos
       $F0     = 0
       $F6     = 0
       $F7     = 0
      <begin>
       pos     = [$C7]
       $F1     = next byte (high part of $F0)
       A       = next byte
       $C7     = pos
       A(word) = ( word.A - word.$F0 ) / byte.$F4
       X       = [$F6]
       [$7F1CF9+X] = word.A
       [$7F1CE9+X] = word.$F0
       [$F6]   = X+2
      <loop 8 times>
       [$39]   = 6
       X       = [$C7]
       
       The table 7F1CF9 is referred by function at C024A7.
       It does adds the first (F9) value to the second (E9).
       Therefore, F9 is the speed of change and E9 is the counter.
       
       But why such pointless math with constant values? No idea.
       If you want to test it, see Lucca's wondershot scene. She
       invokes this event there.

       Here is C++ code that simulates this opcode:

        unsigned char divider = Stream[0];
        for(unsigned pos=1, c=0; c<8; ++c)
        {
            signed short B = Stream[pos++] << 8; 
            signed short A = Stream[pos++] << 8; 
            A = (A-B) / divider; 
            printf("%d %d\n", F0, A);
        }
       

    */

<< "[Mode7Scene:%0:%1:%2]"
    << 0 >> 0xFF
    << 1 >> DeclareByte("0", 0x90, 0x90)
    << 2 >> DeclareWord("1")
    << 4 >> DeclareByte("2")
    //sets $39=1, $3A=paramword, $3D=parambyte $3C=0
    // >> black circle that opens similar to a portal and covers the entire screen (DNL)

<< "[Mode7Scene:%0:%1:%2]"
    << 0 >> 0xFF
    << 1 >> DeclareByte("0", 0x97, 0x97)
    << 2 >> DeclareWord("1")
    << 4 >> DeclareByte("2")
    //sets $39=4, $3A=paramword, $3D=parambyte $3C=0

<< "[Mode7Scene:%0]"
    << 0 >> 0xFF
    << 1 >> DeclareByte("0", 0x00, 0x8F)
    //Calls a Mode 7 scene.
    //Causes data at 0x031513 to be decompressed.
    //A few values:

<< "[Mode7Scene:%0]"
    << 0 >> 0xFF
    << 1 >> DeclareByte("0", 0x91, 0x95)
    // 0x00:// >> highway race
    // 0x01:// >> none
    // 0x02:// >> title screen
    // 0x03:// >> top of black omen, "blurs" into view (Does Not Load new graphics, may not be visible)
    // 0x04:// >> lavos falls to earth
    // 0x0A:// >> fireworks
    // 0x0C:// >> credits over moving star background
    // 0x0D:// >> programmer's ending credits
    // 0x25:// >> lavos summoned to 600AD (DNL)
    // 0x66:// >> Epoch, first person view
    // 0x67:// >> world globe exploding, "But the future refused to change"
    // 0x68:// >> world globe, "But the future refused to change" (short version)
    // 0x69:// >> attract mode highway race
    // 0x80:// >> long wormhole (first warp to 600 A.D.)
    // 0x81:// >> normal wormhole
    // 0x82:// >> quick wormhole
    // 0x89:// >> wormhole to lavos
    // 0x91: //sets $39 = 2
    // 0x92: //may sometimes not progress
             // >> the screen wipe effect used during attract mode (left to right)
    // 0x93: //may sometimes not progress
             // >> the screen wipe effect used during attract mode (right to left, open)
    // 0x94: //may sometimes not progress
             // >> left to right wipe (close)
    // 0x95: //may sometimes not progress
             // >> right to left wipe (close)
    // 0x96: //does not return (resets the system?)
             // >> Reset (see Castle Magus Inner Sanctum)
    // 0x98: //sets $39 = 5
             // >> used by Taban during Moonlight Parade ending
    // 0x99: //sets $39 = 7
             // >> used during Death Peak summit sequence, no noticable effect
    // 0x9A: //sets $39 = 9
             // >> used after Crono revived in Death Peak sequence 
    // 0x9B: //sets $39 = 0xB
             // >> Massive Portal (see Castle Magus Inner Sanctum)
    // 0x9C: //sets $39 = 0xE
             // >> Beam upward (Sunstone) (MAYBE)
    // 0x9D: //sets $54 &= ~2
    // 0x9E: //if $39==0, sets to 0xC. loops if $39!=0xD.
             // >> Reality Distortion (see Castle Magus Inner Sanctum)
    // 0x9F: //calls C28004,A=8
             // >> used in Tesseract
    // 0xA0: // 0xA1: //seems like it halts the system
    // 0xA2: // 0xA3: //seems like it halts the system

<< "[Mode7Scene:%0]"
    << 0 >> 0xFF
    << 1 >> DeclareByte("0", 0x98, 0xA3)

<< "[Mode7Scene:%0]"
    << 0 >> 0xFF
    << 1 >> DeclareByte("0", 0x96, 0x96)

        ;
        } /* Init() */

        void Flush()
        {
            if(!cur_command) return;
            
            cur_command->PutInto(target);
            
            delete cur_command;
            cur_command = NULL;
        }

    public:
        Initialize(OpcodeTree& t) : target(t), cur_command(NULL) { Init(); }
        ~Initialize() { Flush(); }
        
    private:
        OpcodeTree& target;
        Command* cur_command;
        unsigned curpos;
    };

    void FindChoices
        (const OpcodeTree& tree,
         std::list<Command>& choices,
         const unsigned char* data, unsigned length) const
    {
        if(length > 0)
        {
            unsigned char opcode = data[0];
            
            OpcodeTree::maptype::const_iterator i;
            for(i = tree.data.begin(); i != tree.data.end(); ++i)
            {
                if(!i->first.Contains(opcode)) continue;
                FindChoices(i->second, choices, data+1, length-1);
            }
        }
        choices.insert(choices.begin(), tree.choices.begin(), tree.choices.end());
    }

    const std::list<Command> FindChoices
        (const unsigned char* data, unsigned length) const
    {
        std::list<Command> choices;
        FindChoices(OPTree, choices, data, length);
        return choices;
    }
    
public:
    EvCommands()
    {
        Initialize tmp(OPTree);
        //OPTree.Dump();
    }
    
    const EventCode::DecodeResult
    Scan(unsigned offset, const unsigned char* data, unsigned length,
         EventCode::DecodingState& State) const
    {
        std::list<Command> choices = FindChoices(data, length);
        std::list<Command>::const_iterator i;
        
        //fprintf(stderr, "%u choices for %02X\n", choices.size(), data[0]);
        
        for(i=choices.begin(); i!=choices.end(); ++i)
        {
            try
            {
                return i->Scan(offset, data, length, State);
            }
            catch(bool)
            {
            }
        }
        fprintf(stderr, "No choices for %02X\n", data[0]);
        throw false;
    }
    
private:
    OpcodeTree OPTree;
};

namespace
{
    static EvCommands evdata;
}

EventCode::EventCode()
{
}
EventCode::~EventCode()
{
}

const EventCode::DecodeResult
EventCode::DecodeBytes(unsigned offset, const unsigned char* data, unsigned maxlength)
{
    try
    {
        return evdata.Scan(offset, data, maxlength, DecodeState);
    }
    catch(bool)
    {
        DecodeResult result;
        result.code   = wformat(L"[ERROR: No match [offs:%04X] [max:%04X]]", offset, maxlength);
        result.nbytes = 1;
        return result;
    }
}
