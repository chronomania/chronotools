#include <map>
#include <list>
#include <vector>
#include <cstdio>
#include <algorithm>

#include "eventdata.hh"
#include "miscfun.hh"
#include "romaddr.hh"
#include "base62.hh"
#include "base16.hh"
#include "ctcset.hh"

#include "autoptr"

#undef DEBUG_SCAN
#undef DEBUG_FORMAT

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
"017) Game initialization",
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
"144) Crono's Room",
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
const char* Emotion[0x20] = // Poses
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
"{13} Beat Chest (Robo)", // talktalk (Ayla)
"{14} Unknown",
"{15} Right Hand Up",
"{16} Nod",
"{17} Shake Head",
"{18} Unknown",
"{19} D'oh!",
"{1A} Laugh",
"{1B} ?",
"{1C} ?",
"{1D} ?",
"{1E} Right hand",
"{1F} Left hand",
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
    typedef std::map<char, paramholder> parammap;


    /* Prints the string using the format, filling in the parameters. */
    static const std::wstring
        FormatString(const char*const opformat, const parammap& params)
    {
        std::wstring result;
        const char* fmtptr = opformat;
        while(*fmtptr)
        {
            if(*fmtptr == '%')
            {
                char paramname = *++fmtptr; ++fmtptr;
                parammap::const_iterator i = params.find(paramname);
                if(i == params.end())
                {
                    std::fprintf(stderr,
                        "Internal error: Param '%c' not found for '%s'\n",
                        paramname, opformat);
                    continue;
                }
                result += i->second;
                continue;
            }
            result += WcharToAsc(*fmtptr++);
        }
        return result;
    }

    /* Attempts to scan the parameters from the string. Return false if fail. */
    static bool
        ScanParams(const char*const opformat, const std::wstring& string, parammap& params)
    {
        unsigned strpos=0;
        const char* fmtptr = opformat;
        while(strpos < string.size())
        {
            /* Ignore spaces in the input */
            if(string[strpos] == L' ')
            {
                ++strpos; continue;
            }
            
            /* Ignore spaces in the format string */
            while(*fmtptr == ' ') ++fmtptr;
            
            if(*fmtptr == '%')
            {
                /* Read the parameter name */
                char paramname = *++fmtptr; ++fmtptr;
                
                unsigned param_begin = strpos;
                
                enum { mode_unknown,
                       mode_number,
                       mode_operator,
                       mode_dialogaddr,
                       mode_identifier,
                       mode_string } mode = mode_unknown;
                
                for(; strpos < string.size(); ++strpos)
                {
                    wchar_t ch = string[strpos];
                    
                    bool ok=true;
                    switch(mode)
                    {
                        case mode_number:
                            if(!std::isxdigit(WcharToAsc(ch))) ok=false;
                            break;
                        case mode_dialogaddr:
                            if(!std::isalnum(WcharToAsc(ch))) ok=false;
                            break;
                        case mode_identifier:
                            if(ch==L'}') {++strpos;ok=false; }
                            break;
                        case mode_operator: operator_check:
                            if(ch!=L'&' && ch!=L'|'
                            && ch!=L'<' && ch!=L'>'
                            && ch!=L'=' && ch!=L'!')
                                ok=false;
                            break;
                        case mode_string:
                            if(ch==L'"') {++strpos;ok=false; }
                            break;
                        case mode_unknown:
                            if(ch==L'"') {mode=mode_string;break;}
                            if(ch==L'$') {mode=mode_dialogaddr;break;}
                            if(ch==L'{') {mode=mode_identifier;break;}
                            if(std::isxdigit(WcharToAsc(ch))) {mode=mode_number; break;}
                            mode=mode_operator;
                            goto operator_check;
                    }
                    if(!ok) break;
                }
                params[paramname] = string.substr(param_begin, strpos-param_begin);
                continue;
            }
            
            if(*fmtptr != WcharToAsc(string[strpos]))
                return false;
            ++fmtptr;
            ++strpos;
        }
        /* Ignore spaces in the end of the format string */
        while(*fmtptr == ' ') ++fmtptr;
        /* Return 'true' (ok) if the format is now complete (end) */
        return *fmtptr == '\0';
    }
    
    class MemoryAddressConstants
    {
        std::map<unsigned, std::wstring> per_addr;
        std::multimap<std::wstring, unsigned> per_name;
    public:
        void Define(unsigned n, const std::wstring& s)
        {
            per_addr.insert(std::make_pair(n, s));
            per_name.insert(std::make_pair(s, n));
        }
        const std::wstring& Find(unsigned n)
        {
            std::map<unsigned, std::wstring>::const_iterator
                i = per_addr.find(n);
            if(i == per_addr.end()) throw false;
            return i->second;
        }
        unsigned Find(const std::wstring& name) const
        {
            std::multimap<std::wstring, unsigned>::const_iterator
                i = per_name.find(name);
            if(i == per_name.end()) throw false;
            return i->second;
        }
    } MemoryAddressConstants;

    /* Formatting functions */
    static const std::wstring FormatNumeric(unsigned n, unsigned bits)
    {
        try
        {
            return MemoryAddressConstants.Find(n);
        }
        catch(bool)
        {
            return wformat(L"%0*X", bits/4, n);
        }
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
                wchar_t c = getwchar_t((ctchar)byte, cset_8pix);
                if(c != ilseq && c != L'\0')
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

    /* Throws a dummy exception if fails. */
    static unsigned ScanNumeric(const std::wstring& n)
    {
        try
        {
            return MemoryAddressConstants.Find(n);
        }
        catch(bool)
        {
            unsigned offset=0;
            if(n.substr(0, 2) == L"0x") { offset=2; }
            else if(n.substr(0, 1) == L"$") { offset=1; }
            
            unsigned result=0;
            while(offset < n.size())
                if(!CumulateBase16(result, n[offset++]))
                    throw false;
            return result;
        }
    }
    
    static const std::vector<Byte> ScanBlob(const std::wstring& n)
    {
        std::vector<Byte> result;
        if(n.size() < 2) throw false;
        unsigned endpos = n.size()-1;
        if(n[0] != L'"' || n[endpos] != L'"') throw false;
        for(unsigned a=1; a<endpos; ++a)
        {
            if(n[a] == L'[')
            {
                ++a;
                if(a+2 >= endpos || n[a+2] != L']') throw false;
                unsigned code = ScanNumeric(n.substr(a,2));
                if(code > 0xFF) throw false;
                result.push_back(code);
                a += 2;
            }
            else
            {
                ctchar c = getctchar(n[a], cset_8pix);
                if(c == 0 || c >= 0x100) throw false;
                result.push_back(c & 0xFF);
            }
        }
        return result;
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
    static unsigned ScanDialogBegin(const std::wstring& n, unsigned& dialog_begin)
    {
        if(n.size() != 5 || n[0] != L'$') throw false;
        unsigned result=0;
        for(unsigned a=1; a<n.size(); ++a)
            if(!CumulateBase62(result, n[a])) throw false;
        
        dialog_begin = result;
        return ROM2SNESaddr(result);
    }
    static unsigned ScanDialogAddr(const std::wstring& n, unsigned dialog_begin)
    {
        if(n.size() != 5 || n[0] != L'$') throw false;
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
}


namespace
{
    /* Implementing a map type that allows containing ranges
     * of keys.
     * It is simple because we don't worry about overlapping
     * ranges, range extending or range contracting.
     */
    template<typename keytype>
    struct range
    {
        keytype lower, upper; /* Both are inclusive. */
        
        range(keytype l,keytype u): lower(l),upper(u) {}
        
        bool operator< (const range& b) const
        {
            return lower < b.lower;
        }
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
    
        ElemData(unsigned nb, unsigned mi,unsigned ma,unsigned ad,int sh,typetype t=t_trivial)
            : bytepos(0), type(t),
              n_bytes(nb),min(mi),max(ma),add(ad),shift(sh),
              highbit_trick(false) {}
        
        ElemData& DeclareHighbit()
        {
            highbit_trick = true;
            return *this;            
        }
        
        ElemData& SetBytePos(unsigned n) { bytepos = n; return *this; }
        unsigned GetBytePos() const { return bytepos; }
        
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
            
            return FormatNumeric(min, n_bytes*8);
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
        struct ScanResult
        {
            std::vector<unsigned char> bytes;
            unsigned targetpos;
            bool is_goto;

        public:
            explicit ScanResult(unsigned pos): targetpos(pos), is_goto(false) { }
        };
        
        FormatResult Format
            (unsigned offs, const unsigned char* data, unsigned maxlen,
             EventCode::DecodingState& state) const
        {
            FormatResult result;
            
            if(maxlen < bytepos)
            {
#ifdef DEBUG_FORMAT
                fprintf(stderr, "maxlen(%u) bytepos(%u)\n", maxlen, bytepos);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                            fprintf(stderr, "value(%X) & mask(%X)\n", value, mask);
#endif
                            throw false;
                        }
                        value &= ~mask;
                    }
                    
                    /* Check ranges. */
                    if(value < min || value > max)
                    {
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "value(%X) min(%X) max(%X)\n", value, min, max);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
                        throw false;
                    }
                    unsigned length = data[0] | (data[1] << 8);
                    data += 2; maxlen -= 2;
                    if(length < 2) throw false;
                    length -= 2;
                    if(maxlen < length)
                    {
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "blob: maxlen(%u) < length(%u)\n", maxlen, length);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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
#ifdef DEBUG_FORMAT
                        fprintf(stderr, "maxlen(%u) < n_bytes(%u)\n", maxlen, n_bytes);
#endif
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

        unsigned Scan(const std::wstring& s, bool goto_backward) const
        {
            // Scanning function for params with no byte representation
            // Also doubles as parameter verifier.

            switch(type)
            {
                case t_trivial:
                {
                    /* First, read the integer value. */
                    unsigned value = ScanNumeric(s);
#ifdef DEBUG_SCAN
                    const unsigned saved_value = value;
#endif
                    if(value < add) goto trivial_err;

                    /* Unadjust the formatting. */
                    value -= add;
                    /* FIXME: Erased bits should be 0. */
                    if(shift < 0)
                    {
                        // undo right-shifting
                        value <<= -shift;
                    }
                    else if(shift > 0)
                    {
                        unsigned tmp_value = value;
                        value >>= shift;
                        if(tmp_value != (value << shift))
                        {
#ifdef DEBUG_SCAN
                            fprintf(stderr, "low bits shouldn't be set.\n");
#endif
                            throw false;
                        }
                    }
                    
                    /* Check ranges. */
                    if(value < min || value > max)
                    { trivial_err:
#ifdef DEBUG_SCAN
                        fprintf(stderr,
                            "value(%X,%X) add(%X) shift(%d) min(%X) max(%X)\n",
                            saved_value, value, add, shift, min, max);
#endif
                        throw false;
                    }

                    /* When highbit trick is used, there are two rules:
                     * - The input must _not_ have high bit set
                     * - The encoded value _must_ have it.
                     */
                    if(highbit_trick)
                    {
                        unsigned mask = 1 << (n_bytes * 8 - 1);
                        if(value & mask)
                        {
#ifdef DEBUG_SCAN
                            fprintf(stderr, "value(%X) & mask(%X)\n", value, mask);
#endif
                            throw false;
                        }
                        value |= mask;
                    }
                    return value;
                }
                case t_nibble_hi:
                case t_nibble_lo:
                {
                    long value = ScanNumeric(s);
                    /* Check ranges. */
                    if(value < 0 || value > 15)
                    {
#ifdef DEBUG_SCAN
                        fprintf(stderr, "nibble value %ld out of range\n", value);
#endif
                        throw false;
                    }
                    return value;
                }
                case t_orbit:
                case t_andbit:
                {
                    long value = ScanNumeric(s);
                    if(type == t_andbit) value ^= 0xFF;
                    unsigned bit=0;
                    while(value != 1)
                    {
                        if(bit>=7 || !value || (value&1)) throw false;
                        value >>= 1; ++bit;
                    }
                    return bit;
                }
                case t_else:
                case t_loop:
                case t_if:
                {
                    if((type == t_loop) != goto_backward)
                        throw false;
                    return 0;
                }
                case t_operator:
                {
                    return ScanOperator(s);
                }
                case t_textblob:
                case t_blob:
                case t_dialogbegin:
                case t_dialogaddr:
                {
                    // Unhandled
                    break;
                }
            }
            fprintf(stderr, "Internal Error: Unreachable code reached in handling of '%s'\n",
                WstrToAsc(s).c_str());
            throw true;
        }

        ScanResult Scan(bool goto_backward) const
        {
            // Scanning function for values that don't appear in param list

            switch(type)
            {
                case t_trivial:
                {
                    ScanResult result(bytepos);
                    unsigned value = min;
                    for(unsigned n=0; n<n_bytes; ++n)
                        result.bytes.push_back((value >> (n*8)) & 0xFF);
                    return result;
                }
                case t_else:
                case t_loop:
                case t_if:
                {
                    ScanResult result(bytepos);
                    if((type == t_loop) != goto_backward)
                        throw false;
                    result.bytes.push_back(0);
                    result.is_goto = true;
                    return result;
                }
                case t_nibble_hi:
                case t_nibble_lo:
                case t_orbit:
                case t_andbit:
                case t_operator:
                case t_textblob:
                case t_blob:
                case t_dialogbegin:
                case t_dialogaddr:
                    // Unhandled
                    break;
            }
            fprintf(stderr, "Internal Error: Unreachable code reached\n");
            throw true;
        }
        
        ScanResult Scan
            (const std::wstring& s,
             bool goto_backward,
             EventCode::EncodingState& state) const
        {
            // Scanning function for params that
            // have a byte representation and appear in param list
            
            ScanResult result(bytepos);
            
            switch(type)
            {
                case t_trivial:
                {
                    unsigned value = Scan(s, goto_backward);
                    for(unsigned n=0; n<n_bytes; ++n)
                        result.bytes.push_back((value >> (n*8)) & 0xFF);
                    break;
                }
                case t_nibble_hi:
                case t_nibble_lo:
                {
                    unsigned value = Scan(s, goto_backward);
                    if(type == t_nibble_hi) value <<= 4;
                    result.bytes.push_back(value);
                    break;
                }
                case t_orbit:
                case t_andbit:
                {
                    unsigned bit = Scan(s, goto_backward);
                    result.bytes.push_back(bit);
                    break;
                }
                case t_operator:
                {
                    unsigned op = Scan(s, goto_backward);
                    result.bytes.push_back(op);
                    break;
                }
                case t_textblob:
                case t_blob:
                {
                    try
                    {
                        std::vector<unsigned char> blob = ScanBlob(s);
                        unsigned length = blob.size() + 2;
                        result.bytes.push_back(length & 0xFF);
                        result.bytes.push_back(length >> 8);
                        result.bytes.insert(result.bytes.end(), blob.begin(), blob.end());
                    }
                    catch(bool)
                    {
//#ifdef DEBUG_SCAN
                        fprintf(stderr, "'%s' is not a good blob\n", WstrToAsc(s).c_str());
//#endif
                        throw false;
                    }
                    break;
                }
                case t_dialogbegin:
                {
                    unsigned value = ScanDialogBegin(s, state.dialogbegin);
                    for(unsigned n=0; n<3; ++n)
                        result.bytes.push_back((value >> (n*8)) & 0xFF);
                    break;
                }
                case t_dialogaddr:
                {
                    unsigned value = ScanDialogAddr(s, state.dialogbegin);
                    result.bytes.push_back(value);
                    break;
                }

                case t_else:
                case t_loop:
                case t_if:
                {
                    // Unhandled
                    fprintf(stderr, "Internal Error: Unreachable code reached in handling of '%s'\n",
                        WstrToAsc(s).c_str());
                    throw true;
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
    class StringTree;
    
    class Command
    {
    public:
        Command() {}
        explicit Command(const char* fmt) : format(fmt) { }
    
        void Add(ElemData data, unsigned bytepos, char name)
        {
            if(!name) { Add(data, bytepos); return; }
            
            data.SetBytePos(bytepos);
            AddPosData(name, data);
        }
        void Add(ElemData data, unsigned bytepos)
        {
            data.SetBytePos(bytepos);
            AddPosData('\0', data);
        }
        void Add(const ElemData& data, char name)
        {
            AddOtherData(name, data);
        }
        
        void SetSize(unsigned nbytes)
        {
            min_size = nbytes;
        }
        
        void PutInto(OpcodeTree& tree, unsigned bytepos=0)
        {
            /* Encodes the data into the opcode tree
             * for optimized retrieval */
        
            // Find out which of the options defines the range
            // for this byte
            bool found=false;
            
            for(list_t::const_iterator p = pos_data.begin(); p != pos_data.end(); ++p)
            {
                const ElemData& elem = p->second;
                if(elem.KnowRange(bytepos))
                {
                    range<unsigned char> r(elem.GetMin(bytepos), elem.GetMax(bytepos));
                    
                    OpcodeTree::maptype::iterator i = tree.data.find(r);
                    if(i == tree.data.end())
                    {
                        OpcodeTree *subtree = new OpcodeTree;
                        PutInto(*subtree, bytepos+1);
                        tree.data.insert(std::make_pair(r, subtree));
                    }
                    else
                    {
                        OpcodeTree& subtree = *i->second;
                        PutInto(subtree, bytepos+1);
                    }
                    found=true;
                }
            }
            // If there were no specialisations, use the current node.
            if(!found) tree.choices.push_back(*this);
        }
        void PutInto(StringTree& tree, unsigned characterpos=0)
        {
            /* Encodes the data into the character tree
             * for optimized retrieval */
        
            bool found=false;
            
            char c;
            while((c = format[characterpos]) == ' ') ++characterpos;
            
            if(c != '%')
            {
                StringTree::maptype::iterator i = tree.data.find(c);
                if(i == tree.data.end())
                {
                    StringTree *subtree = new StringTree;
                    PutInto(*subtree, characterpos+1);
                    tree.data.insert(std::make_pair(c, subtree));
                }
                else
                {
                    StringTree& subtree = *i->second;
                    PutInto(subtree, characterpos+1);
                }
                found=true;
            }
            // If there were no specialisations, use the current node.
            if(!found) tree.choices.push_back(*this);
        }
        
        const EventCode::DecodeResult
        Format(unsigned offset, const unsigned char* data, unsigned length,
               EventCode::DecodingState& state) const
        {
            parammap params;
            
            EventCode::DecodeResult result;
            
            result.goto_type = EventCode::goto_none;
            
            unsigned nbytes = 1;
            for(list_t::const_iterator p = pos_data.begin(); p != pos_data.end(); ++p)
            {
                const char name      = p->first;
                const ElemData& elem = p->second;
                
                /* Even if it doesn't have a name, it needs to be decoded
                 * to get the opcode length properly.
                 */
                
                ElemData::FormatResult
                    tmp = elem.Format(offset, data, length, state);
                
                if(name)
                {
                    params[name] = tmp.text;
                }
                if(tmp.goto_type != EventCode::goto_none)
                {
                    result.goto_type   = tmp.goto_type;
                    result.goto_target = tmp.goto_target;
                }
                if(tmp.maxoffs > nbytes) nbytes = tmp.maxoffs;
            }

            for(list_t::const_iterator p = other_data.begin(); p != other_data.end(); ++p)
            {
                const char name      = p->first;
                const ElemData& elem = p->second;
                
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
        
        const EventCode::EncodeResult
        Scan(const std::wstring& cmd, bool goto_backward,
             EventCode::EncodingState& state) const
        {
            parammap params;
            if(!ScanParams(format, cmd, params))
            {
                throw false;
            }
            
            EventCode::EncodeResult result;
            
            /* For the parameters which have no byte representation,
             * verify the values of the parameters match.
             */

            for(list_t::const_iterator p = other_data.begin(); p != other_data.end(); ++p)
            {
                const char name      = p->first;
                const ElemData& elem = p->second;
                
                if(!name)
                {
                    fprintf(stderr, "No name on 'other_data'?\n");
                    throw false;
                }
                elem.Scan(params[name], goto_backward);
            }
            
            /* Encode all parameters that contribute to the
             * byte representation.
             */
            for(list_t::const_iterator p = pos_data.begin(); p != pos_data.end(); ++p)
            {
                const char name      = p->first;
                const ElemData& elem = p->second;
                
                ElemData::ScanResult tmp = name
                    ? elem.Scan(params[name], goto_backward, state)
                    : elem.Scan(goto_backward);
                
                if(tmp.is_goto)
                {
                    result.goto_position = tmp.targetpos;
                }
                unsigned needed_size = tmp.targetpos + tmp.bytes.size();
                if(result.result.size() < needed_size)
                {
                    result.result.resize(needed_size);
                }
                
                for(unsigned a=0; a<tmp.bytes.size(); ++a)
                    result.result[tmp.targetpos+a] |= tmp.bytes[a];
            }
            
            return result;
        }
    private:
        void AddPosData(char name, const ElemData& elem)
        {
            pos_data.push_back(std::make_pair(name, elem));
        }
        void AddOtherData(char name, const ElemData& elem)
        {
            other_data.push_back(std::make_pair(name, elem));
        }
        
    private:
        const char* format;
        unsigned min_size;
        typedef std::list<std::pair<char, ElemData> > list_t;
        list_t pos_data;
        list_t other_data;
    };
    
private:
    typedef autoptr<class OpcodeTree> OpcodeTreePtr;
    class OpcodeTree: public ptrable
    {
    public:
        OpcodeTree() { }
    
        typedef simple_rangemap<unsigned char, OpcodeTreePtr> maptype;
        maptype data;
        std::vector<Command> choices;
        
        void Optimize()
        {
            /* Optimize all subtrees */
            for(maptype::iterator i = data.begin(); i != data.end(); ++i)
            {
                i->second->Optimize();
            }
            /* If we now have only 1 subtree and it's not deep,
             * assimilate it. */
            if(data.size() == 1)
            {
                maptype::iterator i = data.begin();
                const OpcodeTree& child = *i->second;
                if(child.data.empty()) // the child must not have children
                {
                    // ok, take the child's commands and delete it
                    choices.insert(choices.end(), child.choices.begin(), child.choices.end());
                    data.erase(i);
                }
            }
        }
        
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
                const OpcodeTreePtr& p = i->second;
                const OpcodeTree& subtree = *p;
                
                std::fprintf(stderr, "%*s", indent, "");
                std::fprintf(stderr, "subtree %02X-%02X:\n", lower,upper);
                subtree.Dump(indent+2);
            }
        }
    };

    typedef autoptr<class StringTree> StringTreePtr;
    class StringTree: public ptrable
    {
    public:
        StringTree() { }
        
        typedef std::map<char, StringTreePtr> maptype;
        maptype data;
        std::vector<Command> choices;
        
        void Optimize()
        {
            /* Optimize all subtrees */
            for(maptype::iterator i = data.begin(); i != data.end(); ++i)
            {
                i->second->Optimize();
            }
            /* If we now have only 1 subtree and it's not deep,
             * assimilate it. */
            if(data.size() == 1)
            {
                maptype::iterator i = data.begin();
                const StringTree& child = *i->second;
                if(child.data.empty()) // the child must not have children
                {
                    // ok, take the child's commands and delete it
                    choices.insert(choices.end(), child.choices.begin(), child.choices.end());
                    data.erase(i);
                }
            }
        }
        
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
                const StringTreePtr& p = i->second;
                const StringTree& subtree = *p;
                
                std::fprintf(stderr, "%*s", indent, "");
                std::fprintf(stderr, "subtree %c:\n", i->first ? i->first : '*');
                subtree.Dump(indent+2);
            }
        }
    };
    
private:
    void FindChoices_aux
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
                FindChoices_aux(*i->second, choices, data+1, length-1);
            }
        }
        choices.insert(choices.begin(), tree.choices.begin(), tree.choices.end());
    }

    const std::list<Command> FindChoices
        (const unsigned char* data, unsigned length) const
    {
        std::list<Command> choices;
        FindChoices_aux(OPTree, choices, data, length);
        return choices;
    }


    void FindChoices_aux
        (const StringTree& tree,
         std::list<Command>& choices,
         const wchar_t* cmd, int length) const
    {
    redo:
        if(length >= 0)
        {
            char ch = length > 0 ? WcharToAsc(cmd[0]) : '\0';
            if(ch == ' ')
            {
                ++cmd; --length;
                goto redo;
            }
            
            StringTree::maptype::const_iterator i = tree.data.find(ch);
            if(i != tree.data.end())
            {
                FindChoices_aux(*i->second, choices, cmd+1, length-1);
            }
        }
        choices.insert(choices.begin(), tree.choices.begin(), tree.choices.end());
    }

    const std::list<Command> FindChoices
        (const std::wstring& cmd, bool goto_backward) const
    {
        std::list<Command> choices;
        FindChoices_aux(STRTree, choices, cmd.data(), cmd.size());
        return choices;
    }
    
public:
    EvCommands()
    {
#include "eventdata.inc"
        OPTree.Optimize();
        STRTree.Optimize();
        //OPTree.Dump();
        //STRTree.Dump();
    }
    
    const EventCode::DecodeResult
    Format(unsigned offset, const unsigned char* data, unsigned maxlength,
           EventCode::DecodingState& State) const
    {
        std::list<Command> choices = FindChoices(data, maxlength);
        std::list<Command>::const_iterator i;
        
        for(i=choices.begin(); i!=choices.end(); ++i)
        {
            try
            {
                return i->Format(offset, data, maxlength, State);
            }
            catch(bool)
            {
            }
        }
        std::string eep;
        for(unsigned a=0; a<6 && a<maxlength; ++a)
            eep += format(" %02X", data[a]);
        
        fprintf(stderr, "No choices for %s\n", eep.c_str());
        throw false;
    }
    
    const EventCode::EncodeResult
    Scan(const std::wstring& cmd, bool goto_backward,
         EventCode::EncodingState& State) const
    {
        std::list<Command> choices = FindChoices(cmd, goto_backward);
        std::list<Command>::const_iterator i;
        
        EventCode::EncodeResult result;
        bool first=true;
        
        //fprintf(stderr, "%u choices for '%s'...\n", choices.size(), WstrToAsc(cmd).c_str());
        
        /* TODO: Thinkable speed optimization:
         *    Sort the choices in the order of expected length
         *    and pick the first one that works.
         */
        
        for(i=choices.begin(); i!=choices.end(); ++i)
        {
            try
            {
                EventCode::EncodeResult tmp = i->Scan(cmd, goto_backward, State);
                //fprintf(stderr, "'%s' ok\n", WstrToAsc(cmd).c_str());
                
                if(first || tmp.result.size() < result.result.size())
                {
                    result = tmp;
                    first  = false;
                }
            }
            catch(bool)
            {
            }
        }
        if(first)
        {
            fprintf(stderr, "No choices for '%s'\n", WstrToAsc(cmd).c_str());
            throw false;
        }
        return result;
    }
    
private:
    OpcodeTree OPTree;
    StringTree STRTree;
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
        return evdata.Format(offset, data, maxlength, DecodeState);
    }
    catch(bool)
    {
        DecodeResult result;
        result.code   = wformat(L"[ERROR: No match [offs:%04X] [max:%04X]]", offset, maxlength);
        result.nbytes = 1;
        return result;
    }
}

const EventCode::EncodeResult
EventCode::EncodeCommand(const std::wstring& cmd, bool goto_backward)
{
    return evdata.Scan(cmd, goto_backward, EncodeState);
}
