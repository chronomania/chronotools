#include <map>
#include <vector>
#include <cstdio>

#include "eventdata.hh"
#include "miscfun.hh"
#include "romaddr.hh"
#include "base62.hh"

typedef unsigned char Byte;

/* Pointers to these events are starting from 0x3CF9F0. */
const char* LocationEventNames[0x201] =
{
"000) Guardia Throneroom",
"001) Bangor Dome",
"002) ",
"003) Trann Dome",
"004) Arris Dome",
"005) ",
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
"{01} Walking ",
"{02} Sprinting",
"{03} Battle   ",
"{04} Unknown  ",
"{05} Asleep   ",
"{06} Running  ",
"{07} Fast Running",
"{08} Defeated",
"{09} Shocked ",
"{0A} Victory ",
"{0B} Unknown ",
"{0C} Determined",
"{0D} Unknown   ",
"{0E} Jump?     ",
"{0F} Unknown   ",
"{10} Shock?    ",
"{11} Standing? ",
"{12} Weak",
"{13} Beat Chest (Robo)",
"{14} Unknown",
"{15} Right Hand Up",
"{16} Nod",
"{17} Shake Head",
"{18} Unknown   ",
"{19} D'oh!     ",
"{1A} Laugh     "
};
const char* ActorNames[0x100] =
{
"{00} Melchior ",
"{01} King Guardia {1000 A.D.}",
"{02} Johnny",
"{03} Queen Leene {blue dress}",
"{04} Tata",
"{05} Toma",
"{06} Kino",
"{07} Chancellor {green} {1000 A.D.}",
"{08} Dactyl",
"{09} Schala",
"{0A} Janus ",
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
"{15} Middle Ages/Present Age villager - waitress   ",
"{16} Middle Ages/Present Age villager - shopkeeper ",
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
"{39} Middle Ages/Present Age villager - old man  ",
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
"{44} Algetty earthbound one - child  ",
"{45} Queen Leene {green dress}",
"{46} Guardia Castle chef",
"{47} Trial judge",
"{48} Gaspar",
"{49} Fiona ",
"{4A} Queen Zeal",
"{4B} Guard {enemy}",
"{4C} Reptite",
"{4D} Kilwala",
"{4E} Blue imp",
"{4F} Middle Ages/Present Age villager - man",
"{50} Middle Ages/Present Age villager - woman",
"{51} G.I. Jogger",
"{52} Millennial Fair visitor - old man",
"{53} Millennial Fair visitor - woman  ",
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
"{5F} Magus statue   ",
"{60} Dreamstone     ",
"{61} Gate Key       ",
"{62} Soda can       ",
"{63} Pendant        ",
"{64} Poyozo doll    ",
"{65} Pink lunch bag ",
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
"{74} Explosion ball ",
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
"{80} Water splash   ",
"{81} Explosion      ",
"{82} Robo power-up sparks",
"{83} Leaves falling/NOT USED",
"{84} 3 coins spinning/NOT USED",
"{85} Hole in the ground",
"{86} Cooking smoke",
"{87} 3 small explosion clouds",
"{88} Wind element spinning   ",
"{89} Water element/NOT USED  ",
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
"{B1} Alfador   ",
"{B2} Time egg  ",
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
"{C6} Millennial Fair visitor - little girl ",
"{C7} Silver Leene's bell",
"{C8} Figure atop Magus' Castle {ending}",
"{C9} Serving tray with drinks",
"{CA} THE END text",
"{CB} Human Glenn {ending cutscene}",
"{CC} Queen Zeal {Death Peak cutscene}",
"{CD} Schala {Death Peak cutscene}",
"{CE} Lavos {Death Peak cutscene} ",
"{CF} Crono {Death Peak cutscene} ",
"{D0} Hironobu Sakaguchi {Programmer's Ending}",
"{D1} Yuji Horii {Programmer's Ending}",
"{D2} Akira Toriyama {Programmer's Ending}",
"{D3} Kazuhiko Aoki {Programmer's Ending} ",
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
"{E2} Blue balloon/NOT USED  ",
"{E3} Pink balloon/NOT USED  ",
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
const char *SoundEffectNames[0x100] =
{
"{00} Cursor selection",
"{01} Invalid cursor selection",
"{02} Falling sprite",
"{03} Pendant reacting",
"{04} Received item   ",
"{05} Activating portal",
"{06} HP/MP restored   ",
"{07} End of Time HP/MP restore bucket",
"{08} Weapon readied",
"{09} Pendant reacting to Zeal throne room door",
"{0A} Flying object",
"{0B} Save point   ",
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
"{2D} UNUSED    ",
"{2E} UNUSED   ",
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
"{3B} UNUSED ",
"{3C} UNUSED",
"{3D} UNUSED",
"{3E} UNUSED  ",
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
"{51} Sealed door opening ",
"{52} Switch pressed",
"{53} Door opened   ",
"{54} Earthquake/rumbling [LOOPS]",
"{55} Gold received",
"{56} Giant doors opened",
"{57} Metal mug put down",
"{58} Unknown [LOOPS] - NOT USED IN EVENTS",
"{59} Metal objects colliding 1 ",
"{5A} Metal objects colliding 2 ",
"{5B} Magic Urn enemy",
"{5C} Exhaust",
"{5D} Unknown [LOOPS] - NOT USED IN EVENTS",
"{5E} Conveyor belt [LOOPS]",
"{5F} Unknown",
"{60} Metal gate crashing",
"{61} Squeak",
"{62} Running [LOOPS]",
"{63} Weapon readied ",
"{64} Poly rolling [LOOPS]",
"{65} Treasure chest opened",
"{66} Unknown",
"{67} Invalid password entry",
"{68} Crane password prompt ",
"{69} Dactyl flying [LOOPS] ",
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
"{74} Pathway opens ",
"{75} Unknown - NOT USED IN EVENTS",
"{76} Unknown - NOT USED IN EVENTS",
"{77} Big explosion",
"{78} Teleport",
"{79} Unknown ",
"{7A} NPC scream",
"{7B} Lightning on 2300 A.D. map",
"{7C} Thunder on 2300 A.D. map  ",
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
"{8D} Sprite lands  ",
"{8E} Collision     ",
"{8F} Bat squeak    ",
"{90} Enemy scream  ",
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
"{9D} Unknown ",
"{9E} Computer display power on",
"{9F} Computer display power off",
"{A0} Typing",
"{A1} Light sirens [LOOPS]",
"{A2} Retinite moving [LOOPS]",
"{A3} Orb enemy blinking",
"{A4} Enemy scream 3",
"{A5} Trial audience cheers",
"{A6} Trial audience boos  ",
"{A7} Enemy sleeping",
"{A8} Pop",
"{A9} Unknown",
"{AA} Enemy startled",
"{AB} Water splash  ",
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
"{BB} Machinery  ",
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
"{CF} Rapid explosions [LOOPS] ",
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
"{E1} Mammon Machine [LOOPS]   ",
"{E2} Lavos 2nd form [LOOPS]   ",
"{E3} Lavos 2nd form defeated  ",
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

const char *SongNames[0x53] =
{
"{00} Silence   ",
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
"{16} 1.23 - A Shot of Crisis  ",
"{17} 1.21 - Kingdom Trial     ",
"{18} 1.02 - Chrono Trigger    ",
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
"{2A} 1.19 - Frog's Theme ",
"{2B} 1.10 - Goodnight    ",
"{2C} 2.08 - Bike Chase   ",
"{2D} 2.04 - People who Threw Away the Will to Live",
"{2E} 2.02 - Mystery of the Past",
"{2F} 2.16 - Underground Sewer  ",
"{30} 1.01 - Presentiment",
"{31} 3.08 - Undersea Palace",
"{32} 3.14 - Last Battle",
"{33} 2.03 - Lab 16's Ruins",
"{34} Inside the Shell",
"{35} Quake",
"{36} 2.21 - Burn!  Bobonga!",
"{37} Wormhole",
"{38} 2.18 - Primitive Mountain",
"{39} 3.13 - World Revolution  ",
"{3A} Lavos Scream",
"{3B} 3.07 - Sealed Door",
"{3C} 1.17 - Silent Light",
"{3D} 2.15 - Fanfare 3         ",
"{3E} 2.13 - The Brink of Time",
"{3F} 3.17 - To Far Away Times",
"{40} 2.23 - Confusing Melody ",
"{41} Hail Magus",
"{42} 1.07 - Gonzales' Song",
"{43} Rain",
"{44} 3.11 - Black Dream",
"{45} 1.12 - Battle 1   ",
"{46} 3.02 - Tyran Castle",
"{47} Fall of Mount Woe  ",
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

/* EV Parameter Handling Functions */
class EvParameterHandler
{
public:
    struct labeldata
    {
        std::string label_name;
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
    
    struct elemdata
    {
        enum { t_byte, t_nibble_hi, t_nibble_lo, t_word, t_long,
               t_7F0200_2, t_7E1000_B, t_7F0000_B, t_7F0000_W,
               t_operator, t_goto,
               t_dialogbegin,
               t_dialogaddr
             } type;
        unsigned bytepos;
        int sign;   //goto
        unsigned offset; //goto
    };
    
    struct structure
    {
        unsigned size;
        typedef std::map<std::string, elemdata> elemmap;
        elemmap elems;
    } structure;
    
    /* A proxy class for std::string
     * To ensure we don't accidentally
     * assign an integer into a string. */
    struct paramholder: public std::string
    {
    private:
        void operator=(unsigned n);
        void operator=(unsigned char n);
        void operator=(char n);
    public:
        paramholder& operator= (const std::string& s)
        { std::string::operator=(s); return *this; }
    };
    
protected:
    typedef std::map<std::string, paramholder> parammap;


    /* Formatting functions */
    static const std::string FormatByte(unsigned n)
    {
        return format("%02X", n);
    }
    static const std::string FormatNibble(unsigned n)
    {
        return format("%X", n);
    }
    static const std::string FormatWord(unsigned n)
    {
        return format("%04X", n);
    }
    static const std::string FormatLong(unsigned n)
    {
        return format("%06X", n);
    }
    const std::string FormatDialogBegin(unsigned n)
    {
        dialog_begin = n = ROM2SNESaddr(n);
        return "$" + EncodeBase62(n, 4);
    }
    const std::string FormatDialogAddr(unsigned n)
    {
        unsigned DialogAddr = dialog_begin + n*2;
        return "$" + EncodeBase62(DialogAddr, 4);
    }
    const std::string FormatGoto(int offset)
    {
        unsigned pointer = offset + dataptr;
        
        std::string Buf = format("{{%04X}}", pointer);
        
        label.label_value = pointer;
        label.label_name  = Buf;
        return Buf;
    }
    const std::string FormatOperator(unsigned char op) const
    {
        static const char* ops[8] = {"==", "!=",
                                     ">", "<",
                                     ">=", "<=",
                                     "&", "|"};
        return ops[op & 7];
    }
    static const std::string FormatData(const std::vector<Byte>& data)
    {
        std::string result;
        result += '"';
        for(unsigned a=0; a<data.size(); ++a)
        {
            result += format("[%02X]", data[a]);
        }
        result += '"';
        return result;
    }

    /* Throws a dummy exception if fails. */
    static unsigned ScanInt(const std::string& n, long max=0xFFFFFF)
    {
        unsigned offset=0;
        int base=16;
        if(n.substr(0, 2) == "0x") { offset=2; base=16; }
        else if(n.substr(0, 1) == "$") { offset=1; base=16; }
        char* endptr;
        long retval = std::strtol(n.c_str()+offset, &endptr, base);
        if(*endptr || retval < 0 || retval > max) throw false;
        return retval;
    }
    static const std::vector<Byte> ScanData(const std::string& n)
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
    const int ScanGoto(const std::string& n, unsigned bytepos, unsigned offset, int sign)
    {
        label.label_name     = n;
        label.label_position = bytepos;
        label.label_forward  = sign>0;
        
        return -offset;
    }
    static unsigned ScanOperator(const std::string& n)
    {
        if(n == "==" || n == "=") return 0;
        if(n == "!=" || n == "<>") return 1;
        if(n == ">") return 2;
        if(n == "<") return 3;
        if(n == ">=") return 4;
        if(n == "<=") return 5;
        if(n == "&") return 6;
        if(n == "|") return 7;
        throw false;
    }
    const unsigned ScanDialogBegin(const std::string& n)
    {
        if(n.size() != 5 || n[0] != '$') throw false;
        unsigned result=0;
        for(unsigned a=1; a<n.size(); ++a)
            if(!CumulateBase62(result, n[a])) throw false;
        
        dialog_begin = result;
        return SNES2ROMaddr(result);
    }
    const unsigned ScanDialogAddr(const std::string& n)
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

    void DeclareByte(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_byte;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareNibbleHi(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_nibble_hi;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareNibbleLo(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_nibble_lo;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareWord(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_word;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareLong(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_long;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareDialogBegin(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_dialogbegin;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareDialogAddr(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_dialogaddr;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void Declare7F0200_2(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_7F0200_2;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void Declare7E1000_B(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_7E1000_B;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void Declare7F0000_B(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_7F0000_B;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void Declare7F0000_W(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_7F0000_W;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareOperator(const std::string& paramname, unsigned bytepos)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_operator;
        tmp.bytepos = bytepos;
        structure.elems[paramname] = tmp;
    }
    void DeclareGoto(const std::string& paramname, unsigned bytepos, int sign, unsigned offset=0)
    {
        elemdata tmp;
        tmp.type    = elemdata::t_goto;
        tmp.bytepos = bytepos;
        tmp.sign    = sign;
        tmp.offset  = offset;
        structure.elems[paramname] = tmp;
    }
    void DeclareSize(unsigned nbytes)
    {
        structure.size = nbytes;
    }

public:
    void SetFormat(const char* fmt) { opformat = fmt; }
    void SetDataPtr(unsigned n)     { dataptr = n;  }
    unsigned GetDataPtr() const     { return dataptr; }
    
    const labeldata QueryGotoUsage() const
    {
        return label;
    }

    /* Tests if the given string matches the given format. Returns true if ok. */
    bool CompareFormat(const std::string& string) const
    {
        parammap dummy;
        return ScanParams(string, dummy);
    }
    
    /* Prints the string using the format, filling in the parameters. */
    const std::string FormatString(const parammap& params) const
    {
        std::string result;
        const char* fmtptr = opformat;
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
            result += *fmtptr++;
        }
        return result;
    }

    /* Attempts to scan the parameters from the string. Return false if fail. */
    bool ScanParams(const std::string& string, parammap& params) const
    {
        unsigned strpos=0;
        const char *fmtptr = opformat;
        while(strpos < string.size())
        {
            if(string[strpos] == ' ') { ++strpos; continue; }
            while(*fmtptr == ' ') ++fmtptr;
            if(*fmtptr == '%')
            {
                std::string paramname;
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

public:
    EvParameterHandler()
    {
    }
    
    virtual ~EvParameterHandler()
    {
    }

public:
    /* Converts parameters to a string. Return value: # bytes eaten */
    virtual unsigned BytesToString(std::string& result,
                                   const Byte* data,
                                   unsigned reserve)
    {
        parammap params;

        if(reserve < structure.size)throw false;
        
        structure::elemmap::const_iterator i;
        for(i=structure.elems.begin(); i!=structure.elems.end(); ++i)
        {
            const Byte* ptr = data + i->second.bytepos;
            //printf("i->second.offset=%u, ptr=%p\n", i->second.offset, ptr);
            
            switch(i->second.type)
            {
                case elemdata::t_byte:
                    params[i->first] = FormatByte(ptr[0]);
                    break;
                case elemdata::t_nibble_hi:
                    params[i->first] = FormatNibble(ptr[0] >> 4);
                    break;
                case elemdata::t_nibble_lo:
                    params[i->first] = FormatNibble(ptr[0] & 15);
                    break;
                case elemdata::t_word:
                    params[i->first] = FormatWord(ptr[0] | (ptr[1] << 8));
                    break;
                case elemdata::t_long:
                    params[i->first] = FormatLong(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16));
                    break;
                case elemdata::t_dialogbegin:
                    params[i->first] = FormatDialogBegin(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16));
                    break;
                case elemdata::t_dialogaddr:
                    params[i->first] = FormatDialogAddr(ptr[0]);
                    break;
                case elemdata::t_7F0200_2:
                    params[i->first] = FormatLong(0x7F0200 + 2*ptr[0]);
                    break;
                case elemdata::t_7E1000_B:
                    params[i->first] = FormatLong(0x7E1000 + ptr[0]);
                    break;
                case elemdata::t_7F0000_B:
                    params[i->first] = FormatLong(0x7F0000 + ptr[0]);
                    break;
                case elemdata::t_7F0000_W:
                    params[i->first] = FormatLong(0x7F0000 + ptr[0] + (ptr[1] << 8));
                    break;
                case elemdata::t_operator:
                    params[i->first] = FormatOperator(ptr[0]);
                    break;
                case elemdata::t_goto:
                    params[i->first] = FormatGoto(ptr[0] * i->second.sign + i->second.offset);
                    break;
            }
        }
        
        result = FormatString(params);
        return structure.size;
    }
    
    /* Converts a string to bytes. Result vector is not cleared. */
    /* Throws an exception (a dummy bool value) if fails (vector untouched). */
    virtual void StringToBytes(const std::string& string,
                               std::vector<Byte>& result)
    {
        parammap params;

        if(!ScanParams(string, params)) throw false;
        
        std::vector<Byte> bytes(structure.size);

        structure::elemmap::const_iterator i;
        for(i=structure.elems.begin(); i!=structure.elems.end(); ++i)
        {
            unsigned offset = i->second.bytepos;
            const std::string& str = params[i->first];
            
            switch(i->second.type)
            {
                case elemdata::t_byte:
                {
                    unsigned n = ScanInt(str, 0xFF);
                    bytes[offset] = n;
                    break;
                }
                case elemdata::t_nibble_hi:
                {
                    unsigned n = ScanInt(str, 0xF);
                    bytes[offset] |= n << 4;
                    break;
                }
                case elemdata::t_nibble_lo:
                {
                    unsigned n = ScanInt(str, 0xF);
                    bytes[offset] |= n;
                    break;
                }
                case elemdata::t_word:
                {
                    unsigned n = ScanInt(str, 0xFFFF);
                    bytes[offset] = n&255;
                    bytes[offset+1] = n>>8;
                    break;
                }
                case elemdata::t_dialogbegin:
                {
                    unsigned n = ScanDialogBegin(str);
                    bytes[offset] = n&255;
                    bytes[offset+1] = (n>>8)&255;
                    bytes[offset+2] = n>>16;
                    break;
                }
                case elemdata::t_dialogaddr:
                {
                    unsigned n = ScanDialogAddr(str);
                    bytes[offset] = n;
                    break;
                }
                case elemdata::t_long:
                {
                    unsigned n = ScanInt(str, 0xFFFFFF);
                    bytes[offset] = n&255;
                    bytes[offset+1] = (n>>8)&255;
                    bytes[offset+2] = n>>16;
                    break;
                }
                case elemdata::t_7F0200_2:
                {
                    int n = ScanInt(str, 0xFFFFFF) - 0x7F0200;
                    if(n < 0 || n > 0x1FF || (n&1)) throw false;
                    bytes[offset] = n/2;
                    break;
                }
                case elemdata::t_7E1000_B:
                {
                    int n = ScanInt(str, 0xFFFF) - 0x7E1000;
                    if(n < 0 || n > 0xFF) throw false;
                    bytes[offset] = n;
                    break;
                }
                case elemdata::t_7F0000_B:
                {
                    int n = ScanInt(str, 0xFFFF) - 0x7E1000;
                    if(n < 0 || n > 0xFFFF) throw false;
                    bytes[offset]   = n&255;
                    bytes[offset+1] = n>>8;
                    break;
                }
                case elemdata::t_7F0000_W:
                {
                    int n = ScanInt(str, 0xFFFF) - 0x7F0000;
                    if(n < 0 || n > 0xFFFF) throw false;
                    bytes[offset]   = n&255;
                    bytes[offset+1] = n>>8;
                    break;
                }
                case elemdata::t_operator:
                {
                    unsigned n = ScanOperator(str);
                    bytes[offset] = n;
                    break;
                }
                case elemdata::t_goto:
                {
                    unsigned n = ScanGoto(str, offset, i->second.offset, i->second.sign);
                    bytes[offset] = n;
                    break;
                }
            }
        }
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
};

namespace
{
    #define BEGIN_EVENT(classname) \
        class classname: public EvParameterHandler { \
        public: \
            static EvParameterHandler* Create() { return new classname; }
               
    #define END_EVENT() };

    BEGIN_EVENT(EvNoParams)
        EvNoParams()
        {
            DeclareSize(0);
        }
    END_EVENT()

    BEGIN_EVENT(EvByteAndTwoNibbles)
        EvByteAndTwoNibbles()
        {
            DeclareByte("0", 0);
            DeclareNibbleHi("1", 1);
            DeclareNibbleLo("2", 1);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvTwoBytes)
        EvTwoBytes()
        {
            DeclareByte("0", 0);
            DeclareByte("1", 1);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvWordAndTwoBytes)
        EvWordAndTwoBytes()
        {
            DeclareWord("0", 0);
            DeclareByte("1", 2);
            DeclareByte("2", 3);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(EvThreeBytes)
        EvThreeBytes()
        {
            DeclareByte("0", 0);
            DeclareByte("1", 1);
            DeclareByte("2", 2);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(EvSixBytes)
        EvSixBytes()
        {
            DeclareByte("0", 0);
            DeclareByte("1", 1);
            DeclareByte("2", 2);
            DeclareByte("3", 3);
            DeclareByte("4", 4);
            DeclareByte("5", 5);
            DeclareSize(6);
        }
    END_EVENT()

    BEGIN_EVENT(EvCommandFE)
        EvCommandFE()
        {
            DeclareByte("0", 0); DeclareByte("1", 1); DeclareByte("2", 2);
            DeclareByte("3", 3); DeclareByte("4", 4); DeclareByte("5", 5);
            DeclareByte("6", 6); DeclareByte("7", 7); DeclareByte("8", 8);
            DeclareByte("9", 9); DeclareByte("10",10); DeclareByte("11",11);
            DeclareByte("12",12); DeclareByte("13",13); DeclareByte("14",14);
            DeclareByte("15",15); DeclareByte("16",16);
            DeclareSize(17);
        }
    END_EVENT()

    BEGIN_EVENT(EvCommandsE4andE5)
        EvCommandsE4andE5()
        {
            DeclareByte("l", 0);
            DeclareByte("t", 1);
            DeclareByte("r", 2);
            DeclareByte("b", 3);
            DeclareByte("x", 4);
            DeclareByte("y", 5);
            DeclareByte("f", 6);
            DeclareSize(7);
        }
    END_EVENT()

    BEGIN_EVENT(EvTwoWords)
        EvTwoWords()
        {
            DeclareWord("0", 0);
            DeclareWord("1", 2);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(EvPosiGoto)
        EvPosiGoto()
        {
            DeclareGoto("0", 0, +1, 0);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(EvNegaGoto)
        EvNegaGoto()
        {
            DeclareGoto("0", 0, -1, 0);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(EvOneByte)
        EvOneByte()
        {
            DeclareByte("0", 0);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(EvOneWord)
        EvOneWord()
        {
            DeclareWord("0", 0);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvOneLong)
        EvOneLong()
        {
            DeclareLong("0", 0);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(EvLongWithByte)
        EvLongWithByte()
        {
            DeclareLong("long", 0);
            DeclareByte("byte", 3);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(EvLongWithWord)
        EvLongWithWord()
        {
            DeclareLong("long", 0);
            DeclareWord("word", 3);
            DeclareSize(5);
        }
    END_EVENT()

    BEGIN_EVENT(EvLongWith7F0200)
        EvLongWith7F0200()
        {
            DeclareLong("long", 0);
            Declare7F0200_2("addr", 3);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(Ev7E1000)
        Ev7E1000()
        {
            Declare7E1000_B("0", 1);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(Ev7F0200)
        Ev7F0200()
        {
            Declare7F0200_2("0", 0);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(EvByteWith7F0200)
        EvByteWith7F0200()
        {
            DeclareByte("byte", 0);
            Declare7F0200_2("addr", 1);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvWordWith7F0200)
        EvWordWith7F0200()
        {
            DeclareByte("word", 0);
            Declare7F0200_2("addr", 2);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(Ev7F0000)
        Ev7F0000()
        {
            Declare7F0000_B("0", 0);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(EvByteWith7F0000)
        EvByteWith7F0000()
        {
            DeclareByte("byte", 0);
            Declare7F0000_W("addr", 1);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(EvDialogBegin)
        EvDialogBegin()
        {
            DeclareDialogBegin("0", 0);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(EvDialogAddr)
        EvDialogAddr()
        {
            DeclareDialogAddr("0", 0);
            DeclareSize(1);
        }
    END_EVENT()

    BEGIN_EVENT(EvDialogAddrWithByte)
        EvDialogAddrWithByte()
        {
            DeclareDialogAddr("0", 0);
            DeclareByte("1", 1);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvIf12)
        EvIf12()
        {
            Declare7F0200_2("addr", 0);
            DeclareByte("value", 1);
            DeclareOperator("op", 2);
            DeclareGoto("label", 3, +1, 3);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(EvIf13)
        EvIf13()
        {
            Declare7F0200_2("addr", 0);
            DeclareWord("value", 1);
            DeclareOperator("op", 3);
            DeclareGoto("label", 4, +1, 4);
            DeclareSize(5);
        }
    END_EVENT()

    BEGIN_EVENT(EvIf14And15)
        EvIf14And15()
        {
            Declare7F0200_2("addr1", 0);
            Declare7F0200_2("addr2", 1);
            DeclareOperator("op", 2);
            DeclareGoto("label", 3, +1, 3);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(EvIf16)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            if(reserve < 4)throw false;
        
            parammap params;
            unsigned addr = 0x7E0000 + data[0];
            if(data[2] & 0x80) addr += 0x100;
            params["addr"]  = FormatLong(addr);
            params["value"] = FormatByte(data[1]);
            params["op"]    = FormatOperator(data[2]);
            params["label"] = FormatGoto(data[3] + 3);
            result = FormatString(params);
            return 4;
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            
            long addr = ScanInt(params["addr"]) - 0x7E0000;
            if(addr < 0) throw false;
            if(addr > 0x1FF) throw false;
            unsigned value = ScanInt(params["value"], 0xFF);
            unsigned op    = ScanOperator(params["op"]);
            int label      = ScanGoto(params["label"], 3, 3, +1);
            if(addr&0x100) op |= 0x80;
            result.push_back(addr&0xFF);
            result.push_back(value);
            result.push_back(op);
            result.push_back(label);
        }
    END_EVENT()

    BEGIN_EVENT(EvTwo7F0200)
        EvTwo7F0200()
        {
            Declare7F0200_2("0", 0);
            Declare7F0200_2("1", 1);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvFour7F0200)
        EvFour7F0200()
        {
            Declare7F0200_2("0", 0);
            Declare7F0200_2("1", 1);
            Declare7F0200_2("2", 2);
            Declare7F0200_2("3", 3);
            DeclareSize(4);
        }
    END_EVENT()

    BEGIN_EVENT(Ev7F0000And7F0200)
        Ev7F0000And7F0200()
        {
            Declare7F0000_W("0", 0);
            Declare7F0200_2("1", 2);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(Ev7F0200And7F0000)
        Ev7F0200And7F0000()
        {
            Declare7F0200_2("0", 0);
            Declare7F0000_W("1", 1);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(EvIfOneByte)
        EvIfOneByte()
        {
            DeclareByte("0",     0);
            DeclareGoto("label", 1, +1, 1);
            DeclareSize(2);
        }
    END_EVENT()

    BEGIN_EVENT(EvIfOneWord)
        EvIfOneWord()
        {
            DeclareWord("0",     0);
            DeclareGoto("label", 2, +1, 2);
            DeclareSize(3);
        }
    END_EVENT()

    BEGIN_EVENT(EvCommands65And66)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            if(reserve < 2)throw false;
        
            parammap params;
            params["byte"] = FormatByte(data[0] & 0x7F);
            
            unsigned addr = 0x7E0000 + data[1] + (data[0] & 0x80)*2;
            params["addr"]  = FormatLong(addr);

            result = FormatString(params);
            return 2;
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            
            long addr = ScanInt(params["addr"]) - 0x7E0000;
            if(addr < 0) throw false;
            if(addr > 0x1FF) throw false;
            unsigned byte  = ScanInt(params["byte"], 0x7F);
            if(addr & 0x100) byte |= 0x80;
            
            result.push_back(addr&0xFF);
            result.push_back(byte);
        }
    END_EVENT()

    BEGIN_EVENT(EvCommand4E)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            if(reserve < 5)throw false;
        
            parammap params;
            params["long"]  = FormatLong(data[0] | (data[1] << 8) | (data[2] << 16));
            unsigned length = (data[3] | (data[4] << 8)) - 2;

            if(reserve < 5+length)throw false;
        
            std::vector<Byte> blob(data+5, data+5+length);
            params["data"] = FormatData(blob);
            result = FormatString(params);
            return 5+length;
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            unsigned lval = ScanInt(params["long"], 0xFFFFFF);
            if(lval < 0x7E0000 || lval > 0x7FFFFF) throw false;
            std::vector<Byte> blob = ScanData(params["data"]);
            unsigned length = blob.size() + 2;
            result.push_back(lval&255);
            result.push_back((lval>>8)&255);
            result.push_back(lval>>16);
            result.push_back(length&255);
            result.push_back(length>>8);
            result.insert(result.end(), blob.begin(), blob.end());
        }
    END_EVENT()

    BEGIN_EVENT(EvCommandF1)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            if(reserve < 1)throw false;
        
            parammap params;
            params["index"]    = FormatByte(data[0]);
            if(data[0])
            {
                if(reserve < 2)throw false;
        
                params["duration"] = data[0] ? FormatByte(data[1]) : "";
                result = FormatString(params);
                return 2;
            }
            else
            {
                params["duration"] = "";
                result = FormatString(params);
                return 1;
            }
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            
            unsigned index = ScanInt(params["index"], 0xFF);
            unsigned dura  = 0;
            if(index) dura = ScanInt(params["duration"], 0xFF);
            
            result.push_back(index);
            if(index)
                result.push_back(dura);
        }
    END_EVENT()

    BEGIN_EVENT(EvLoopAnimation)
        EvLoopAnimation()
        {
            DeclareByte("0", 0);
            DeclareByte("1", 1);
            DeclareSize(2);
            
            // LoopAnimation.
            
            /*
                X = $6D
                A = $7F0B01,X
                If(A != 0)
                {
                    @2F09
                    --A
                    if(== #0)
                    {
                        @2F1B
                        A = #0
                        $7F0B01+X = A
                        $1601+X = A
                        $1681+X = 0
                        A = $1600+X
                        if(== #$FF)
                        {
                            @2F32
                            $1680+X = #0
                            A = 0
                        }
                        else
                        {
                            @2F2E
                            A = 1
                        }
                        @2F37
                        $1780+X = A
                        X = Y+3       //opcode + 2 params
                        RETURN
                    }
                    @2F0C
                    X = Y + #1
                    A = $7F2001+X //byte
                    X = $6D
                    If(== $1781+X)
                    {
                        @2F6E->
                    }
                    @2F48->
                }
                else
                {
                    @2F40
                    X = Y + #1
                    A = $7F2001+X //byte
                    X = $6D
                    @2F48 ->
                }
                @2F48
                $1781+X = A
                $1681+X = 0
                $1601+X = 0
                A = $1780+X
                if(Zero)
                {
                    @2F56
                    $1680+X = #$FF
                }
                @2F5B
                $1780+X = #2
                X = Y+2
                A = $7F2001+X //byte
                A++
                X = $6D
                $7F0B01+X = A
                @2F6E
                X = Y        //loop
                RETURN
            */
        }
    END_EVENT()

    BEGIN_EVENT(EvCommand2E)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            parammap params;
            
            if(reserve < 4)throw false;
        
            unsigned byte1 = data[0];
            unsigned byte2 = data[1];
            unsigned byte3 = data[2];
            unsigned byte4 = data[3];
            
            params["0"] = FormatByte(byte1);
            
            switch(byte1 & 0xF0)
            {
                case 0x80:
                {
                    /* Sets palettes. byte2=colour index, blob=data. */
                    unsigned length = (byte3 | (byte4 << 8)) - 2;
                    if(reserve < 4+length)throw false;
    
                    std::vector<Byte> blob(data+4, data+4+length);

                    params["1"] = FormatNibble(byte2 >> 4); // palette number
                    params["2"] = FormatNibble(byte2 & 15); // starting colour
                    params["3"] = FormatData(blob);
                    result = FormatString(params);
                    return 4 + length;
                }
                case 0x40: case 0x50:
                {
                    if(reserve < 5)throw false;
                    // calls function 4B2C.
                    //   if false, ignores 6 bytes (including opcode).
                    //   if true:
                    //     stores return value to $520+y and $526+y
                    //     stores byte2 to $521+y
                    //     stores byte3 to $522+y
                    //     stores byte4 to $DD
                    //     stores byte5 to $525+y
                    //     stores 0 to $524+y
                    //     stores 8 to $523+y
                    //     stores byte4 to $527+y nibbleswapped
                    //     stores byte4 to $528+y nibbleswapped some other way(??)
                    
                    params["1"] = FormatWord(byte2 | (byte3 << 8));
                    params["2"] = FormatByte(byte4);
                    params["3"] = FormatByte(data[4]);
                    result = FormatString(params);
                    return 5;
                }
                default:
                {
                    params["1"] = FormatByte(byte2);
                    params["2"] = "";
                    params["3"] = "";
                    result = FormatString(params);
                    return 2;
                }
           }
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            
            unsigned byte1 = ScanInt(params["0"], 0xFF);
            
            switch(byte1 & 0x80)
            {
                case 0x80:
                {
                    std::vector<Byte> blob = ScanData(params["3"]);
                    unsigned byte2 = (ScanInt(params["1"],15)<<4) | ScanInt(params["2"],15);
                    unsigned length = blob.size() + 2;

                    result.push_back(byte1);
                    result.push_back(byte2);
                    result.push_back(length&255);
                    result.push_back(length>>8);
                    result.insert(result.end(), blob.begin(), blob.end());
                    break;
                }
                case 0x40: case 0x50:
                {
                    unsigned val = ScanInt(params["1"], 0xFFFF);
                    unsigned byte4 = ScanInt(params["2"], 0xFF);
                    unsigned byte5 = ScanInt(params["3"], 0xFF);
                    result.push_back(byte1);
                    result.push_back(val&255);
                    result.push_back(val>>8);
                    result.push_back(byte4);
                    result.push_back(byte5);
                    break;
                }
                default:
                {
                    unsigned byte2 = ScanInt(params["1"], 0xFF);
                    result.push_back(byte1);
                }
            }
        }
    END_EVENT()

    BEGIN_EVENT(EvCommand88)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            parammap params;
            
            if(reserve < 1)throw false;

            unsigned byte1 = data[0];
            if(!byte1)
            {
                // Moves something (?)
            Default:
                params["0"] = FormatByte(byte1);
                params["1"] = "";
                params["2"] = "";
                params["3"] = "";
                result = FormatString(params);
                return 1;
            }
            if(byte1 == 0x20 || byte1 == 0x30)
            {
                if(reserve < 3)throw false;
                params["0"] = FormatByte(byte1);
                params["1"] = FormatByte(data[1]);
                params["2"] = FormatByte(data[2]);
                params["3"] = "";
                result = FormatString(params);
                return 3;
            }
            if(byte1 >= 0x40 && byte1 <= 0x5F)
            {
                if(reserve < 4)throw false;
                params["0"] = FormatByte(byte1);
                params["1"] = FormatByte(data[1]);
                params["2"] = FormatByte(data[2]);
                params["3"] = FormatByte(data[3]);
                result = FormatString(params);
                return 4;
            }
            if(byte1 >= 0x80 && byte1 <= 0x8F)
            {
                if(reserve < 3)throw false;

                unsigned length = data[1] - 2;

                if(reserve < 3+length)throw false;

                std::vector<Byte> blob(data+3, data+3+length);
                params["0"] = FormatByte(byte1);
                params["1"] = FormatByte(data[2]);
                params["2"] = FormatData(blob);
                params["3"] = "";
                result = FormatString(params);
                return 3 + length;
            }
            goto Default;
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            
            unsigned byte1 = ScanInt(params["0"], 0xFF);
            
            if(!byte1)
            {
            Default:
                result.push_back(byte1);
                return;
            }
            if(byte1 == 0x20 || byte1 == 0x30)
            {
                unsigned byte2 = ScanInt(params["1"], 0xFF);
                unsigned byte3 = ScanInt(params["2"], 0xFF);
                result.push_back(byte1);
                result.push_back(byte2);
                result.push_back(byte3);
                return;
            }
            if(byte1 >= 0x40 && byte1 <= 0x5F)
            {
                unsigned byte2 = ScanInt(params["1"], 0xFF);
                unsigned byte3 = ScanInt(params["2"], 0xFF);
                unsigned byte4 = ScanInt(params["3"], 0xFF);
                result.push_back(byte1);
                result.push_back(byte2);
                result.push_back(byte3);
                result.push_back(byte4);
                return;
            }
            if(byte1 >= 0x80 && byte1 <= 0x8F)
            {
                unsigned byte2 = ScanInt(params["1"], 0xFF);
                std::vector<Byte> blob = ScanData(params["2"]);
                unsigned length = blob.size() + 2;

                result.push_back(byte1);
                result.push_back(byte2);
                result.push_back(length&255);
                result.insert(result.end(), blob.begin(), blob.end());
            }
            goto Default;
        }
    END_EVENT()

    BEGIN_EVENT(EvMode7Scene)
        virtual unsigned BytesToString(std::string& result, const Byte* data, unsigned reserve)
        {
            parammap params;
            
            if(reserve < 1)throw false;

            unsigned byte1 = data[0];
            switch(byte1)
            {
                case 0x90:
                case 0x97:
                    if(reserve < 4)throw false;
                    params["0"] = FormatByte(byte1);
                    params["1"] = FormatWord(data[1] | (data[2] << 8));
                    params["2"] = FormatByte(data[3]);
                    result = FormatString(params);
                    return 4;
                default:
                    params["0"] = FormatByte(byte1);
                    params["1"] = "";
                    params["2"] = "";
                    result = FormatString(params);
                    return 1;
            }
        }
        virtual void StringToBytes(const std::string& string, std::vector<Byte>& result)
        {
            parammap params;
            if(!ScanParams(string, params)) throw false;
            
            unsigned byte1 = ScanInt(params["0"], 0xFF);
            switch(byte1)
            {
                case 0x90: //sets $39=1, $3A=paramword, $3D=parambyte $3C=0
                           // - black circle that opens similar to a portal and covers the entire screen (DNL)
                case 0x97: //sets $39=4, $3A=paramword, $3D=parambyte $3C=0
                {
                    unsigned p1 = ScanInt(params["1"], 0xFFFF);
                    unsigned p2 = ScanInt(params["2"], 0xFF);
                    result.push_back(byte1);
                    result.push_back(p1&255);
                    result.push_back((p1>8)&255);
                    result.push_back(p2);
                    return;
                }
                default:
                //Calls a Mode 7 scene.
                //Causes data at 0x031513 to be decompressed.
                //A few values:
                case 0x00:// - highway race
                case 0x01:// - none
                case 0x02:// - title screen
                case 0x03:// - top of black omen, "blurs" into view (Does Not Load new graphics, may not be visible)
                case 0x04:// - lavos falls to earth
                case 0x0A:// - fireworks
                case 0x0C:// - credits over moving star background
                case 0x0D:// - programmer's ending credits
                case 0x25:// - lavos summoned to 600AD (DNL)
                case 0x66:// - Epoch, first person view
                case 0x67:// - world globe exploding, "But the future refused to change"
                case 0x68:// - world globe, "But the future refused to change" (short version)
                case 0x69:// - attract mode highway race
                case 0x80:// - long wormhole (first warp to 600 A.D.)
                case 0x81:// - normal wormhole
                case 0x82:// - quick wormhole
                case 0x89:// - wormhole to lavos
                case 0x91: //sets $39 = 2
                case 0x92: //may sometimes not progress
                           // - the screen wipe effect used during attract mode (left to right)
                case 0x93: //may sometimes not progress
                           // - the screen wipe effect used during attract mode (right to left, open)
                case 0x94: //may sometimes not progress
                           // - left to right wipe (close)
                case 0x95: //may sometimes not progress
                           // - right to left wipe (close)
                case 0x96: //does not return (resets the system?)
                           // - Reset (see Castle Magus Inner Sanctum)
                case 0x98: //sets $39 = 5
                           // - used by Taban during Moonlight Parade ending
                case 0x99: //sets $39 = 7
                           // - used during Death Peak summit sequence, no noticable effect
                case 0x9A: //sets $39 = 9
                           // - used after Crono revived in Death Peak sequence 
                case 0x9B: //sets $39 = 0xB
                           // - Massive Portal (see Castle Magus Inner Sanctum)
                case 0x9C: //sets $39 = 0xE
                           // - Beam upward (Sunstone) (MAYBE)
                case 0x9D: //sets $54 &= ~2
                case 0x9E: //if $39==0, sets to 0xC. loops if $39!=0xD.
                           // - Reality Distortion (see Castle Magus Inner Sanctum)
                case 0x9F: //calls C28004,A=8
                           // - used in Tesseract
                case 0xA0: case 0xA1: //seems like it halts the system
                case 0xA2: case 0xA3: //seems like it halts the system
                {
                    result.push_back(byte1);
                    return ;
                }
            }
        }
    END_EVENT()
}

static const struct
{
    EvParameterHandler* (*Create)(void);
    const char* format;
} EvCommands[256] =
{
    { /*00*/ EvNoParams::Create,            "[Unknown00]" },
    { /*01*/ EvNoParams::Create,          "[Crash:01]" },
    { /*02*/ EvByteAndTwoNibbles::Create,   "[Ev02:%0:%1:%2]" },
    { /*03*/ EvByteAndTwoNibbles::Create,   "[Ev03:%0:%1:%2]" },
    { /*04*/ EvByteAndTwoNibbles::Create,   "[Ev04:%0:%1:%2]" },
    { /*05*/ EvByteAndTwoNibbles::Create,   "[Ev05:%0:%1:%2]" },
    { /*06*/ EvByteAndTwoNibbles::Create,   "[Ev06:%0:%1:%2]" },
    { /*07*/ EvTwoBytes::Create,            "[Ev07:%0:%1]" },
    { /*08*/ EvNoParams::Create,            "[NPCSetEventFlag:1]" },
    { /*09*/ EvNoParams::Create,            "[NPCSetEventFlag:0]" },
    { /*0A*/ EvOneByte::Create,             "[RemoveObj:%0]" },
    { /*0B*/ Ev7E1000::Create,              "[SetHighBit:%0]" },
    { /*0C*/ Ev7E1000::Create,              "[ClearHighBit:%0]" },
    { /*0D*/ EvOneByte::Create,             "[NPCMoveProps:%0]" },
    { /*0E*/ EvOneByte::Create,             "[SetBits1C81:%0]" },
    { /*0F*/ EvNoParams::Create,            "[NPCFacingUp]" },
    { /*10*/ EvPosiGoto::Create,            "[Goto:%0]" },
    { /*11*/ EvNegaGoto::Create,            "[Goto:%0]" },
    { /*12*/ EvIf12::Create,                "[Goto:%label [UnlessB:%addr %op %value]]" },
    { /*13*/ EvIf13::Create,                "[Goto:%label [UnlessW:%addr %op %value]]" },
    { /*14*/ EvIf14And15::Create,           "[Goto:%label [UnlessB:%addr1 %op %addr2]]" },
    { /*15*/ EvIf14And15::Create,           "[Goto:%label [UnlessW:%addr1 %op %addr2]]" },
    { /*16*/ EvIf16::Create,                "[Goto:%label [UnlessB:%addr %op %value]]" },
    { /*17*/ EvNoParams::Create,            "[NPCFacingDown]" },
    { /*18*/ EvIfOneByte::Create,           "[Goto:%label [IfStorylinePoint:%0]]" },
    { /*19*/ Ev7F0200::Create,              "[LoadResult:%0]" },
    { /*1A*/ EvIfOneByte::Create,           "[Goto:%label [UnlessResultIs:%0]]" },
    { /*1B*/ EvNoParams::Create,            "[NPCFacingLeft]" },
    { /*1C*/ Ev7F0000::Create,              "[LoadResult:%0]" },
    { /*1D*/ EvNoParams::Create,            "[NPCFacingRight]" },
    { /*1E*/ EvOneByte::Create,             "[NPCFacingUp:%0]" },
    { /*1F*/ EvOneByte::Create,             "[NPCFacingDown:%0]" },
    { /*20*/ Ev7F0200::Create,              "[PCStore:%0]" },
    { /*21*/ EvThreeBytes::Create,          "[Unknown21:%0:%1:%2]" },
    { /*22*/ EvThreeBytes::Create,          "[Unknown22:%0:%1:%2]" },
    { /*23*/ EvByteWith7F0200::Create,      "[Unknown23:%addr:%byte]" },
    { /*24*/ EvByteWith7F0200::Create,      "[Unknown24:%addr:%byte]" },
    { /*25*/ EvOneByte::Create,             "[NPCFacingLeft:%0]" },
    { /*26*/ EvOneByte::Create,             "[NPCFacingRight:%0]" },
    { /*27*/ EvIfOneByte::Create,           "[Goto:%label [IfEnemySomethingZeroAt:%0]]" },
    { /*28*/ EvTwoBytes::Create,            "[Unknown28:%0:%1]" },
    { /*29*/ EvOneByte::Create,             "[ReptiteEndingText:%0]" },
    { /*2A*/ EvNoParams::Create,            "[OrB:54:04]" },
    { /*2B*/ EvNoParams::Create,            "[OrB:54:08]" },
    { /*2C*/ EvTwoBytes::Create,            "[Unknown2C:%0:%1]" },
    { /*2D*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessF8 == 00]]" },
    { /*2E*/ EvCommand2E::Create,           "[PaletteSet:%0:%1:%2:%3]" },
    { /*2F*/ EvTwoBytes::Create,            "[Unknown2F:%0:%1]" },
    { /*30*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessF8 & 02]]" },
    { /*31*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessF8 & 80]]" },
    { /*32*/ EvNoParams::Create,            "[OrB:54:10]" },
    { /*33*/ EvOneByte::Create,             "[PaletteSomething:%0]" },
    { /*34*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonPressed:A]]" },
    { /*35*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonPressed:B]]" },
    { /*36*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonPressed:X]]" },
    { /*37*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonPressed:Y]]" },
    { /*38*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonPressed:L]]" },
    { /*39*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonPressed:R]]" },
    { /*3A*/ EvNoParams::Create,          "[Crash:3A]" },
    { /*3B*/ EvOneByte::Create,             "[Unused3B:%0]" },
    { /*3C*/ EvOneByte::Create,             "[Unused3C:%0]" },
    { /*3D*/ EvNoParams::Create,          "[Crash:3D]" },
    { /*3E*/ EvNoParams::Create,          "[Crash:3E]" },
    { /*3F*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonStatus:A]]" },
    { /*40*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonStatus:B]]" },
    { /*41*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonStatus:X]]" },
    { /*42*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonStatus:Y]]" },
    { /*43*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonStatus:L]]" },
    { /*44*/ EvPosiGoto::Create,            "[Goto:%0 [UnlessButtonStatus:R]]" },
    { /*45*/ EvNoParams::Create,          "[Crash:45]" },
    { /*46*/ EvNoParams::Create,          "[Crash:46]" },
    { /*47*/ EvOneByte::Create,             "[PokeTo6B:%0]" },
    { /*48*/ EvLongWith7F0200::Create,      "[LetB:%addr:%long]" },
    { /*49*/ EvLongWith7F0200::Create,      "[LetW:%addr:%long]" },
    { /*4A*/ EvLongWithByte::Create,        "[LetB:%long:%byte]" },
    { /*4B*/ EvLongWithWord::Create,        "[LetW:%long:%word]" },
    { /*4C*/ EvLongWith7F0200::Create,      "[LetB:%long:%addr]" },
    { /*4D*/ EvLongWith7F0200::Create,      "[LetW:%long:%addr]" },
    { /*4E*/ EvCommand4E::Create,           "[StringStore:%long:%data]" },
    { /*4F*/ EvByteWith7F0200::Create,      "[LetB:%addr:%byte]" },
    { /*50*/ EvWordWith7F0200::Create,      "[LetW:%addr:%word]" },
    { /*51*/ EvTwo7F0200::Create,           "[LetB:%1:%0]" },
    { /*52*/ EvTwo7F0200::Create,           "[LetW:%1:%0]" },
    { /*53*/ Ev7F0000And7F0200::Create,     "[LetB:%1:%0]" },
    { /*54*/ Ev7F0000And7F0200::Create,     "[LetW:%1:%0]" },
    { /*55*/ Ev7F0200::Create,              "[GetStorylineCounter:%0]" },
    { /*56*/ EvByteWith7F0000::Create,      "[LetB:%addr:%byte]" },
    { /*57*/ EvNoParams::Create,            "[PCLoad:0]" },
    { /*58*/ Ev7F0200And7F0000::Create,     "[LetB:%1:%0]" },
    { /*59*/ Ev7F0200And7F0000::Create,     "[LetW:%1:%0]" },
    { /*5A*/ EvOneByte::Create,             "[SetStorylineCounter:%0]" },
    { /*5B*/ EvByteWith7F0200::Create,      "[AddB:%addr:%byte]" },
    { /*5C*/ EvNoParams::Create,            "[PCLoad:1]" },
    { /*5D*/ EvTwoBytes::Create,            "[Unused5D:%0:%1]" },
    { /*5E*/ EvTwo7F0200::Create,           "[AddW:%1:%0]" },
    { /*5F*/ EvByteWith7F0200::Create,      "[SubB:%addr:%byte]" },
    { /*60*/ EvWordWith7F0200::Create,      "[SubW:%addr:%word]" },
    { /*61*/ EvTwo7F0200::Create,           "[SubB:%1:%0]" },
    { /*62*/ EvNoParams::Create,            "[PCLoad:2]" },
    { /*63*/ EvByteWith7F0200::Create,      "[SetBit:%addr:%byte]" },
    { /*64*/ EvByteWith7F0200::Create,      "[ClearBit:%addr:%byte]" },
    { /*65*/ EvCommands65And66::Create,     "[SetBit:%addr:%byte]" },
    { /*66*/ EvCommands65And66::Create,     "[ClearBit:%addr:%byte]" },
    { /*67*/ EvByteWith7F0200::Create,      "[AndB:%addr:%byte]" },
    { /*68*/ EvNoParams::Create,            "[PCLoad:3]" },
    { /*69*/ EvByteWith7F0200::Create,      "[OrB:%addr:%byte]" },
    { /*6A*/ EvNoParams::Create,            "[PCLoad:4]" },
    { /*6B*/ EvByteWith7F0200::Create,      "[XorB:%addr:%byte]" },
    { /*6C*/ EvNoParams::Create,            "[PCLoad:5]" },
    { /*6D*/ EvNoParams::Create,            "[PCLoad:6]" },
    { /*6E*/ EvNoParams::Create,          "[Crash:6E]" },
    { /*6F*/ EvByteWith7F0200::Create,      "[ShrB:%addr:%byte]" },
    { /*70*/ EvNoParams::Create,          "[Crash:70]" },
    { /*71*/ Ev7F0200::Create,              "[IncB:%0]" },
    { /*72*/ Ev7F0200::Create,              "[IncW:%0]" },
    { /*73*/ Ev7F0200::Create,              "[DecB:%0]" },
    { /*74*/ EvNoParams::Create,          "[Crash:74]" },
    { /*75*/ Ev7F0200::Create,              "[LetB:%0:1]" },
    { /*76*/ Ev7F0200::Create,              "[LetW:%0:1]" },
    { /*77*/ Ev7F0200::Create,              "[LetB:%0:0]" },
    { /*78*/ EvNoParams::Create,          "[Crash:78]" },
    { /*79*/ EvNoParams::Create,          "[Crash:79]" },
    { /*7A*/ EvThreeBytes::Create,          "[NPCJump:%0:%1:%2]" },
    { /*7B*/ EvTwoWords::Create,            "[Unused7B:%0:%1]" },
    { /*7C*/ EvOneByte::Create,             "[SetObjectDrawingFor:%0:on]" },  // Sets 1A81 for given obj to "1"
    { /*7D*/ EvOneByte::Create,             "[SetObjectDrawingFor:%0:off]" }, // Sets 1A81 for given obj to "0"
    { /*7E*/ EvNoParams::Create,            "[SetObjectDrawing:hide]" },
    { /*7F*/ Ev7F0200::Create,              "[GetRandom:%0]" },
    { /*80*/ EvOneByte::Create,             "[PCLoad_IfInParty:%0]" },
    { /*81*/ EvOneByte::Create,             "[PCLoad:%0]" },
    { /*82*/ EvOneByte::Create,             "[NPCLoad:%0]" },
    { /*83*/ EvTwoBytes::Create,            "[LoadEnemy:%0:%1]" },
    { /*84*/ EvOneByte::Create,             "[NPCSolidProps:%0]" },
    { /*85*/ EvNoParams::Create,          "[Crash:85]" },
    { /*86*/ EvNoParams::Create,          "[Crash:86]" },
    { /*87*/ EvOneByte::Create,             "[Unknown87:%0]" },
    { /*88*/ EvCommand88::Create,           "[GFXCommand88:%0:%1:%2:%3]" },
    { /*89*/ EvOneByte::Create,             "[NPCSetSpeed:%0]" }, // Sets 1A00 from const
    { /*8A*/ Ev7F0200::Create,              "[NPCSetSpeed:%0]" }, // Sets 1A00 from var
    { /*8B*/ EvTwoBytes::Create,            "[SetObjectCoord:%0:%1]" },
    { /*8C*/ EvTwoBytes::Create,            "[Unknown8C:%0:%1]" },
    { /*8D*/ EvTwoWords::Create,            "[SetObjectCoord2:%0:%1]" },
    { /*8E*/ EvOneByte::Create,             "[NPCHide:%0]" },
    { /*8F*/ EvOneByte::Create,             "[Unknown8F:%0]" },
    { /*90*/ EvNoParams::Create,            "[SetObjectDrawing:on]" },
    { /*91*/ EvNoParams::Create,            "[SetObjectDrawing:off]" },
    { /*92*/ EvTwoBytes::Create,            "[MoveObject:%0:%1]" },
    { /*93*/ EvNoParams::Create,          "[Crash:93]" },
    { /*94*/ EvOneByte::Create,             "[Unknown94:%0]" },
    { /*95*/ EvOneByte::Create,             "[NPCSetInviteFlag:%0]" },
    { /*96*/ EvTwoBytes::Create,            "[NPCMove:%0:%1]" },
    { /*97*/ EvTwoBytes::Create,            "[Unknown97:%0:%1]" },
    { /*98*/ EvTwoBytes::Create,            "[Unknown98:%0:%1]" },
    { /*99*/ EvTwoBytes::Create,            "[Unknown99:%0:%1]" },
    { /*9A*/ EvThreeBytes::Create,          "[Unknown9A:%0:%1:%2]" },
    { /*9B*/ EvNoParams::Create,          "[Crash:9B]" },
    { /*9C*/ EvTwoBytes::Create,            "[Unknown9C:%0:%1]" },
    { /*9D*/ EvTwo7F0200::Create,           "[Unknown9D:%0:%1]" },
    { /*9E*/ EvOneByte::Create,             "[Unknown9E:%0]" },
    { /*9F*/ EvOneByte::Create,             "[Unknown9F:%0]" },
    { /*A0*/ EvTwoBytes::Create,            "[NPCMoveAnimated:%0:%1]" },
    { /*A1*/ EvTwo7F0200::Create,           "[UnknownA1:%0:%1]" },
    { /*A2*/ EvNoParams::Create,          "[Crash:A2]" },
    { /*A3*/ EvNoParams::Create,          "[Crash:A3]" },
    { /*A4*/ EvNoParams::Create,          "[Crash:A4]" },
    { /*A5*/ EvNoParams::Create,          "[Crash:A5]" },
    { /*A6*/ EvOneByte::Create,             "[NPCSetFacing:%0]" },
    { /*A7*/ Ev7F0200::Create,              "[NPCSetFacing:%0]" },
    { /*A8*/ EvOneByte::Create,             "[NPCSetRelativeFacing:%0]" },
    { /*A9*/ Ev7F0200::Create,              "[NPCSetRelativeFacing:%0]" },
    { /*AA*/ EvOneByte::Create,             "[Animation:%0:1]" },
    { /*AB*/ EvOneByte::Create,             "[AnimationWait:%0]" },
    { /*AC*/ EvOneByte::Create,             "[PlayStaticAnimation:%0]" },
    { /*AD*/ EvOneByte::Create,             "[Pause:%0]" },
    { /*AE*/ EvNoParams::Create,            "[Animation:0:0]" },
    { /*AF*/ EvNoParams::Create,            "[UnknownAF]" },
    { /*B0*/ EvNoParams::Create,            "[ForeverUnknownAF]" },
    { /*B1*/ EvNoParams::Create,            "[LoopBreak]" },
    { /*B2*/ EvNoParams::Create,            "[LoopEnd]" },
    { /*B3*/ EvNoParams::Create,            "[Animation:0:1]" },
    { /*B4*/ EvNoParams::Create,            "[Animation:1:1]" },
    { /*B5*/ EvOneByte::Create,             "[ForeverUnknown94:%0]" },
    { /*B6*/ EvOneByte::Create,             "[ForeverNPCSetInviteFlag:%0]" },
    { /*B7*/ EvLoopAnimation::Create,       "[LoopAnimation:%0:%1]" },
    { /*B8*/ EvDialogBegin::Create,         "[DialogSetTable:%0]" },
    { /*B9*/ EvNoParams::Create,            "[Pause:250ms]" },
    { /*BA*/ EvNoParams::Create,            "[Pause:500ms]" },
    { /*BB*/ EvDialogAddr::Create,          "[DialogDisplay:%0]" },
    { /*BC*/ EvNoParams::Create,            "[Pause:1000ms]" },
    { /*BD*/ EvNoParams::Create,            "[Pause:2000ms]" },
    { /*BE*/ EvNoParams::Create,          "[Crash:BE]" },
    { /*BF*/ EvNoParams::Create,          "[Crash:BF]" },
    { /*C0*/ EvDialogAddrWithByte::Create,  "[DialogAsk:%0:%1]" },       //stores to "result" (7F0A80+X)
    { /*C1*/ EvDialogAddr::Create,          "[DialogDisplayTop:%0]" },
    { /*C2*/ EvDialogAddr::Create,          "[DialogDisplayBottom:%0]" },
    { /*C3*/ EvDialogAddrWithByte::Create,  "[DialogAskTop:%0:%1]" },    //stores to "result" (7F0A80+X)
    { /*C4*/ EvDialogAddrWithByte::Create,  "[DialogAskBottom:%0:%1]" }, //stores to "result" (7F0A80+X)
    { /*C5*/ EvNoParams::Create,          "[Crash:C5]" },
    { /*C6*/ EvNoParams::Create,          "[Crash:C6]" },
    { /*C7*/ Ev7F0200::Create,              "[AddItem:%0]" },
    { /*C8*/ EvOneByte::Create,             "[SpecialDialog:%0]" },
    { /*C9*/ EvIfOneByte::Create,           "[Goto:%label [UnlessHasItem:%0]]" },
    { /*CA*/ EvOneByte::Create,             "[GiveItem:%0]" },
    { /*CB*/ EvOneByte::Create,             "[StealItem:%0]" },
    { /*CC*/ EvIfOneWord::Create,           "[Goto:%label [UnlessHasGold:%0]]" },
    { /*CD*/ EvOneWord::Create,             "[GiveGold:%0]" },
    { /*CE*/ EvOneWord::Create,             "[StealGold:%0]" },
    { /*CF*/ EvIfOneByte::Create,           "[Goto:%label [UnlessHasMember:%0]]" },
    { /*D0*/ EvOneByte::Create,             "[GiveMember:%0]" },
    { /*D1*/ EvOneByte::Create,             "[StealMember:%0]" },
    { /*D2*/ EvIfOneByte::Create,           "[Goto:%label [UnlessHasActiveMember:%0]" },
    { /*D3*/ EvOneByte::Create,             "[GiveActiveMember:%0]" },
    { /*D4*/ EvOneByte::Create,             "[UnactivateMember:%0]" },
    { /*D5*/ EvTwoBytes::Create,            "[EquipMember:%0:%1]" },
    { /*D6*/ EvOneByte::Create,             "[StealActiveMember:%0]" },
    { /*D7*/ EvByteWith7F0200::Create,      "[GetItemAmount:%byte:%addr]" },
    { /*D8*/ EvOneWord::Create,             "[StartBattle:%0]" },
    { /*D9*/ EvSixBytes::Create,            "[PartyMove:%0:%1:%2:%3:%4:%5]" },
    { /*DA*/ EvNoParams::Create,            "[PartySetFollow]" },
    { /*DB*/ EvNoParams::Create,          "[Crash:DB]" },
    { /*DC*/ EvWordAndTwoBytes::Create,     "[PartyTeleportDC:%0:%1:%2]" },
    { /*DD*/ EvWordAndTwoBytes::Create,     "[PartyTeleportDD:%0:%1:%2]" },
    { /*DE*/ EvWordAndTwoBytes::Create,     "[PartyTeleportDE:%0:%1:%2]" },
    { /*DF*/ EvWordAndTwoBytes::Create,     "[PartyTeleportDF:%0:%1:%2]" },
    { /*E0*/ EvWordAndTwoBytes::Create,     "[PartyTeleportE0:%0:%1:%2]" },
    { /*E1*/ EvWordAndTwoBytes::Create,     "[PartyTeleportE1:%0:%1:%2]" },
    { /*E2*/ EvFour7F0200::Create,          "[UnknownE2:%0:%1:%2:%3]" },
    { /*E3*/ EvOneByte::Create,             "[SetExploreMode:%0]" },
    { /*E4*/ EvCommandsE4andE5::Create,     "[UnknownE4:%l:%t:%r:%b:%x:%y:%f]" },
    { /*E5*/ EvCommandsE4andE5::Create,     "[UnknownE5:%l:%t:%r:%b:%x:%y:%f]" },
    { /*E6*/ EvWordAndTwoBytes::Create,     "[ScrollLayers:%0:%1:%2]" },
    { /*E7*/ EvTwoBytes::Create,            "[ScrollScreen:%0:%1]" },
    { /*E8*/ EvOneByte::Create,             "[PlaySound:%0]" },
    { /*E9*/ EvNoParams::Create,          "[Crash:E9]" },
    { /*EA*/ EvOneByte::Create,             "[PlaySong:%0]" },
    { /*EB*/ EvTwoBytes::Create,            "[SetVolume:%0:%1]" },
    { /*EC*/ EvThreeBytes::Create,          "[SoundCommand:%0:%1:%2]" },
    { /*ED*/ EvNoParams::Create,            "[WaitSilence]" },
    { /*EE*/ EvNoParams::Create,            "[WaitSongEnd]" },
    { /*EF*/ EvNoParams::Create,          "[Crash:EF]" },
    { /*F0*/ EvOneByte::Create,             "[FadeOutScreen:%0]" },
    { /*F1*/ EvCommandF1::Create,           "[BrightenScreen:%index:%duration]" },
    { /*F2*/ EvNoParams::Create,            "[FadeOutScreen]" },
    { /*F3*/ EvNoParams::Create,            "[UnknownF3]" },
    { /*F4*/ EvOneByte::Create,             "[ShakeScreen:%0]" },
    { /*F5*/ EvNoParams::Create,          "[Crash:F5]" },
    { /*F6*/ EvNoParams::Create,          "[Crash:F6]" },
    { /*F7*/ EvNoParams::Create,          "[Crash:F7]" },
    { /*F8*/ EvNoParams::Create,            "[HealHPandMP]" },   // does opF9 and opFA
    { /*F9*/ EvNoParams::Create,            "[HealHP]" },        // calls C28004,A=6
    { /*FA*/ EvNoParams::Create,            "[HealMP]" },        // calls C28004,A=7
    { /*FB*/ EvNoParams::Create,          "[Crash:FB]" },
    { /*FC*/ EvNoParams::Create,          "[Crash:FC]" },
    { /*FD*/ EvNoParams::Create,          "[Crash:FD]" },
    { /*FE*/ EvCommandFE::Create,           "[SetGeometry:%0:%1:%2:%3:%4:%5:%6:%7:%8:%9:%10:%11:%12:%13:%14:%15:%16]" },
    { /*FF*/ EvMode7Scene::Create,          "[Mode7Scene:%0:%1:%2]" }
};

EventCode::EventCode() : ev(NULL)
{
}
EventCode::~EventCode()
{
    delete ev; ev=NULL;
}

void EventCode::InitDecode(unsigned offset, unsigned char opcode)
{
    if(ev) delete ev; ev=NULL;
    
    ev = EvCommands[opcode].Create();

    ev->SetDataPtr(offset);
    ev->SetFormat(EvCommands[opcode].format);
}

EventCode::DecodeResult
EventCode::DecodeBytes(const unsigned char* data, unsigned maxlength)
{
    DecodeResult result;
    
    if(!ev)
    {
        std::fprintf(stderr, "EventCode::DecodeBytes: No ev\n");
        std::fflush(stderr); std::fflush(stdout);
        throw false;
    }
    
    try
    {
        unsigned offs = ev->GetDataPtr();
        std::string line;
        
        unsigned size = ev->BytesToString(line, data, maxlength);
        
        result.code   = line;
        result.nbytes = size;
        
        EvParameterHandler::labeldata tmp = ev->QueryGotoUsage();
        result.label_name  = tmp.label_name;
        result.label_value = tmp.label_value;
    }
    catch(bool)
    {
        result.code   = "[ERROR: Not enough bytes]";
        result.nbytes = 0;
    }
    return result;
}
