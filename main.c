#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#endif

// Enums
#define inventoryCapacity 100
#define NUM_TRIGGERS 4
#define MAX_LEVEL 100

typedef enum {
    isAlive = 0,
    hasAxe = 1,
    hasKey1 = 2,
    cyclopsKilled = 3
} triggerNums;

typedef enum {
    slime = 0,
    goblin = 1,
    cyclops = 2,
    randomEnemy = -1
} enemyNum;

typedef enum {
    consumable,
    equipment,
    keyItem,
    ingredient
} itemType;

typedef enum {
    helmet = 0,
    armour = 1,
    weapon = 2,
    accessory = 3
} equipmentType;

// Structs
typedef struct {
    char* name;
    int itemType;
    union {
        struct { char* description; int amount; char* effect; int effectAmount; } consumable;
        struct { int type; int hp, def, atk, agility, acc; bool isEquipped; } equipment;
        struct { char* description; } keyItem;
        struct { int amount; } ingredient;
    } data;
} item;

typedef struct {
    item items[inventoryCapacity];
    int itemCount;
} inventory;

typedef struct {
    bool isPoisoned;
    bool isParalysed;
    bool isFrozen;
    bool isBurning;
} status;

typedef struct {
    int hp;
    int def;
    int atk;
    int acc;
    int agility;
    status status;
} stats;

typedef struct scene scene;
typedef struct {
    inventory* inv;
    bool gameTriggers[NUM_TRIGGERS];
    int lvl;
    int xp;
    int xen;
    int baseXen;
    stats stat;
    stats baseStats;
    struct {
        item helmet;
        item armour;
        item weapon;
        item accessory;
    } equipment;
} player;

typedef struct scene {
    char* description;
    bool* choiceConditions;
    int* choiceIds;
    scene** nextScenePerChoice;
    int numChoices;
    int sceneNo;
    char* choices[];
} scene;

typedef struct {
    char* name;
    int dmg;
    int acc;
    int lvlReq;
    char* debuff;
} move;


typedef struct {
    char* name;
    int xenreq;       // REQUIRED XEN
    int basedamage;   
    char* chant;      
    char* type; 
    status debuff;    // Status 
} Spell;


Spell allSpells[] = {
    { 
        "Fireball", 
        15,                             // Xen cost
        30,                             
        "quiver-crimson tear-ripple",   // Chant phrase
        "fire", 
        {false, false, false, true}     // Causes Burn
    },
    { 
        "Spark", 
        10, 
        20, 
        "supplant-white flash-polarity", 
        "thunder", 
        {false, true, false, false}     // Causes paralysis
    }
};


typedef struct {
    char* name;
    int xpSeed;
    move moveset[4];
    int lvl;
    char* type;
    stats stat;
    inventory drops;
    int dropRate[inventoryCapacity];
} enemy;

// Item arrays
item consumables[] = {
    {"Health Potion", consumable, {.consumable = {"Restores 50 HP", 1, "hpRecovery", 50}}},
    {"Mana Potion", consumable, {.consumable = {"Restores 30 xen", 1, "xenRecovery", 30}}},
    {"Poison Cure", consumable, {.consumable = {"Cures poison", 1, "curePoison", 0}}},
    {"Paralysis Cure", consumable, {.consumable = {"Cures paralysis", 1, "cureParalysis", 0}}},
    {"Burn Cure", consumable, {.consumable = {"Cures burn", 1, "cureBurn", 0}}},
    {"Elixir", consumable, {.consumable = {"Restores all HP and Xen", 1, "fullRecovery", 0}}}
};
item equipments[] = {
    {"Rusty Axe", equipment, {.equipment = {weapon, 0, 0, 10, 0, 100, false}}},
    {"Leather Armor", equipment, {.equipment = {armour, 5, 3, 0, 0, 0, false}}}
};
item keyItems[] = {
    {"Key1", keyItem, {.keyItem = {"A key to unlock the gate."}}}
};

enemy allEnemy[] = {
    {
        "slime", 50,
        { {"blob attack", 20, 100, 1, "NULL"}, {"electrify", 10, 70, 1, "prs"}, {"acid throw", 10, 70, 1, "psn"}, {"slimy touch", 10, 70, 0, "brn"} },
        1, "normal",
        { 25, 15, 20, 100, 20, (status){ false, false, false, false } },
        .drops = { {0},  0}, {0}
    },
    {
        "goblin", 70,
        { {"bite", 20, 100, 1, "NULL"}, {"spear jab", 30, 50, 1, "NULL"}, {"enrage", 0, 100, 1, "atkBuff"}, {"posion arrow", 20, 80, 0, "psn"} },
        1, "normal",
        { 15, 15, 25, 100, 20, (status){ false, false, false, false } },
        .drops = { {0},  0}, {0}
    },
    {
        "cyclops", 90,
        { {"club attack", 100, 10, 1, "NULL"}, {"body slam", 50, 30, 1, "NULL"}, {"enrage", 0, 100, 1, "atkBuff"}, {"laser eye", 0, 100, 0, "brn"} },
        1, "dark",
        { 150, 15, 5, 100, 10, (status){ false, false, false, false } },
        .drops = { {0},  0}, {0}
    }
};

// Function declarations
player* createPlayer();
inventory* createInventory();
void addItem(inventory* inv, item newItem);
void removeItemFromInventory(char* itemName, inventory* inv);
void displayInventory(inventory* inv, player* p);
scene* createScene(int numChoices, char* description, char** choices, bool* choiceConditions, int* choiceIds, scene** nextScenePerChoice, int sceneNo);
scene* processChoice(scene* currentScene, player* p, int choiceIndex);
void displayScene(scene* currentScene, player* p);
void updateSceneConditions(scene* currentScene, player* p);
void freeScene(scene* s);
void freePlayer(player* p);
void freeEnemy(enemy* e);
enemy* createEnemy(player* p, int enemyIndex);
void init_combat(player* p, enemy* e);
void type(const char* format, ...);
void lvlUp(player* p);
void equipItem(player* p, item* item);
void unequipItem(player* p, int equipmentType);
void useItem(player* p, item* item);
void useItemPanel(player* p, inventory* inv);

// Function implementations
inventory* createInventory() {
    inventory* inv = (inventory*)malloc(sizeof(inventory));
    if (inv != NULL) {
        inv->itemCount = 0;
        for (int i = 0; i < inventoryCapacity; i++) {
            inv->items[i].name = NULL;
            inv->items[i].itemType = consumable; // Default to avoid uninitialized access
            inv->items[i].data.consumable.description = NULL;
            inv->items[i].data.consumable.effect = NULL;
            inv->items[i].data.consumable.amount = 0;
            inv->items[i].data.consumable.effectAmount = 0;
        }
    }
    return inv;
}
void addItem(inventory* inv, item newItem) {
    if (!inv || !newItem.name) {
        type("Error: Invalid inventory or item name\n");
        return;
    }
    if (inv->itemCount < inventoryCapacity) {
        for (int i = 0; i < inv->itemCount; i++) {
            if (strcmp(inv->items[i].name, newItem.name) == 0) {
                switch (inv->items[i].itemType) {
                    case consumable:
                        inv->items[i].data.consumable.amount += newItem.data.consumable.amount;
                        return;
                    case equipment:
                        type("%s already exists as equipment.\n", inv->items[i].name);
                        return;
                    case keyItem:
                        type("%s already exists as a key item.\n", inv->items[i].name);
                        return;
                    case ingredient:
                        inv->items[i].data.ingredient.amount += newItem.data.ingredient.amount;
                        type("Updated %s amount to %d.\n", inv->items[i].name, inv->items[i].data.ingredient.amount);
                        return;
                }
            }
        }
        item* dest = &inv->items[inv->itemCount];
        dest->name = strdup(newItem.name);
        if (!dest->name) {
            type("Error: Failed to allocate item name\n");
            return;
        }
        dest->itemType = newItem.itemType;
        switch (newItem.itemType) {
            case consumable:
                dest->data.consumable.description = strdup(newItem.data.consumable.description ? newItem.data.consumable.description : "");
                dest->data.consumable.amount = newItem.data.consumable.amount;
                dest->data.consumable.effect = strdup(newItem.data.consumable.effect ? newItem.data.consumable.effect : "");
                dest->data.consumable.effectAmount = newItem.data.consumable.effectAmount;
                if (!dest->data.consumable.description || !dest->data.consumable.effect) {
                    free(dest->name);
                    free(dest->data.consumable.description);
                    free(dest->data.consumable.effect);
                    type("Error: Failed to allocate consumable fields\n");
                    return;
                }
                break;
            case equipment:
                dest->data.equipment = newItem.data.equipment; // Struct copy is safe
                break;
            case keyItem:
                dest->data.keyItem.description = strdup(newItem.data.keyItem.description ? newItem.data.keyItem.description : "");
                if (!dest->data.keyItem.description) {
                    free(dest->name);
                    type("Error: Failed to allocate key item description\n");
                    return;
                }
                break;
            case ingredient:
                dest->data.ingredient.amount = newItem.data.ingredient.amount;
                break;
        }
        inv->itemCount++;
    } else {
        type("Inventory is full!\n");
    }
}

void useItem(player* p, item* item) {
    if (!p || !item->name) {
        type("Error: Invalid player or item name\n");
        return;
    }
    if (item->itemType != consumable) {
        type("Error: Item is not consumable\n");
        return;
    }
    if (item->data.consumable.amount <= 0) {
        type("You have no %s left to use.\n", item->name);
        return;
    }
    
    if (strcmp(item->data.consumable.effect, "hpRecovery") == 0) {
        p->stat.hp += item->data.consumable.effectAmount;
        int healedAmount = (p->stat.hp > p->baseStats.hp) ? (p->baseStats.hp - (p->stat.hp - item->data.consumable.effectAmount)) : item->data.consumable.effectAmount; 
        if (p->stat.hp > p->baseStats.hp) p->stat.hp = p->baseStats.hp;
        type("You used %s and restored %d HP!\n", item->name, healedAmount);
    } 
    else if (strcmp(item->data.consumable.effect, "xenRecovery") == 0) {
        p->xen += item->data.consumable.effectAmount;
        int recoveredAmount = (p->xen > p->baseXen) ? (p->baseXen - (p->xen - item->data.consumable.effectAmount)) : item->data.consumable.effectAmount;
        if (p->xen > p->baseXen) p->xen = p->baseXen;
        type("You used %s and restored %d Xen!\n", item->name, recoveredAmount);
    } 
    else if (strcmp(item->data.consumable.effect, "curePoison") == 0) {
        if (p->stat.status.isPoisoned) {
            p->stat.status.isPoisoned = false;
            type("You used %s and cured your poison!\n", item->name);
        } else {
            type("You are not poisoned.\n");
        }
    } else if (strcmp(item->data.consumable.effect, "cureParalysis") == 0) {
        if (p->stat.status.isParalysed) {
            p->stat.status.isParalysed = false;
            type("You used %s and cured your paralysis!\n", item->name);
        } else {
            type("You are not paralysed.\n");
        }
    } 
    else if (strcmp(item->data.consumable.effect, "cureBurn") == 0) {
        if (p->stat.status.isBurning) {
            p->stat.status.isBurning = false;
            type("You used %s and cured your burn!\n", item->name);
        } else {
            type("You are not burning.\n");
        }
    } 
    else if (strcmp(item->data.consumable.effect, "fullRecovery") == 0) {
        p->stat.hp = p->baseStats.hp;
        p->xen = p->baseXen; 
        type("You used %s and restored all your HP and Xen!\n", item->name);
    }
    else {
        type("Unknown item effect: %s\n", item->data.consumable.effect);
        return;
    }
    removeItemFromInventory(item->name, p->inv);
}

void useItemPanel(player* p, inventory* inv) {
    if (!p || !inv) {
        type("Error: Null player or inventory\n");
        return;
    }
    if (inv->itemCount == 0) {
        type("Inventory is empty.\n");
        return;
    }
    for (int i = 0; i< inv->itemCount; i++) {
        if (inv->items[i].itemType == consumable) {
            type("%d. %s (Amount: %d, Description: %s, Effect: %s)\n", 
                 i + 1, inv->items[i].name, 
                 inv->items[i].data.consumable.amount, 
                 inv->items[i].data.consumable.description, 
                 inv->items[i].data.consumable.effect);
        }
    }
    type("Enter the number of the item you want to use (or 0 to exit):\n");
    int i  = -1;
    while (i < 0 || i > inv->itemCount) {scanf(" %d", &i); if(i < 0 || i > inv->itemCount) type("Invalid choice. Please enter a number between 1 and %d or 0 to exit.\n", inv->itemCount);}
    if (i == 0) {
        return;
    }
    useItem(p, &inv->items[i - 1]);
    return;
}

void removeItemFromInventory(char* itemName, inventory* inv) {
    if (!inv || !itemName) {
        type("Error: Null inventory or item name\n");
        return;
    }
    if (inv->itemCount == 0) {
        type("Inventory is empty\n");
        return;
    }
    for (int i = 0; i < inv->itemCount; i++) {
        if (strcmp(inv->items[i].name, itemName) == 0) {
            switch (inv->items[i].itemType) {
                case consumable:
                    if(inv->items[i].data.consumable.amount > 1) {
                        inv->items[i].data.consumable.amount--;
                        return;
                    } else {
                        free(inv->items[i].name);
                        free(inv->items[i].data.consumable.description);
                        free(inv->items[i].data.consumable.effect);
                    }
                    break;
                case equipment:
                    free(inv->items[i].name);
                    break;
                case keyItem:
                    free(inv->items[i].name);
                    free(inv->items[i].data.keyItem.description);
                    break;
                case ingredient:
                if(inv->items[i].data.ingredient.amount > 1) {
                    inv->items[i].data.ingredient.amount--;
                    return;
                }
                else {
                    free(inv->items[i].name);
                }
                    break;
            }
            for (int j = i; j < inv->itemCount - 1; j++) {
                inv->items[j] = inv->items[j + 1];
            }
            inv->itemCount--;
            return;
        }
    }
    type("Item not in inventory\n");
}

void displayInventory(inventory* inv, player* p) {
    if (!inv || !p) {
        type("No inventory to display.\n");
        return;
    }
    if (inv->itemCount == 0) {
        type("Inventory is empty.\n");
        return;
    }
    while (true) {
        type("Your Inventory:\n1. Consumables\n2. Equipment\n3. Key Items\n4. Ingredients\n5. Exit\n");
        char input[10];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            type("Error reading input.\n");
            return;
        }
        input[strcspn(input, "\n")] = '\0';
        int choice = atoi(input);
        if (choice < 1 || choice > 5) {
            type("Invalid choice. Please enter 1-5.\n");
            continue;
        }
        bool hasItems = false;
        switch (choice) {
            case 1:
                int consumableNum = 0;
                type("Consumables:\n");
                for (int i = 0; i < inv->itemCount; i++) {
                    if (inv->items[i].itemType == consumable) {
                        hasItems = true;
                        type("%d. %s (Amount: %d, Description: %s, Effect: %s)\n",
                             consumableNum + 1, inv->items[i].name,
                             inv->items[i].data.consumable.amount,
                             inv->items[i].data.consumable.description,
                             inv->items[i].data.consumable.effect);
                        consumableNum++;
                    }
                }
                if (!hasItems) {type("No consumables.\n"); break;}
                type("Do you want to use an item? (y/n)\n");
                char useChoice[10];
                if (fgets(useChoice, sizeof(useChoice), stdin) == NULL) {
                    type("Error reading input.\n");
                    return;
                }
                useChoice[strcspn(useChoice, "\n")] = '\0';
                if (strcmp(useChoice, "y") == 0 || strcmp(useChoice, "Y") == 0) {
                    type("Enter the number of the item to use:\n");
                    char useInput[10];
                    if (fgets(useInput, sizeof(useInput), stdin) == NULL) {
                        type("Error reading input.\n");
                        return;
                    }
                    useInput[strcspn(useInput, "\n")] = '\0';
                    int useIndex = atoi(useInput) - 1;
                    if (useIndex < 0 || useIndex >= consumableNum) {
                        type("Invalid item number.\n");
                    } else {
                        consumableNum = 0; 
                        for(int i = 0; i < inv->itemCount; i++) {
                            if (inv->items[i].itemType == consumable) {
                                consumableNum++;
                                if (consumableNum == useIndex + 1) {
                                    useItem(p, &inv->items[i]);
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            case 2:
                int equipmentNum = 0;
                type("Equipment:\n");
                for (int i = 0; i < inv->itemCount; i++) {
                    if (inv->items[i].itemType == equipment) {
                        hasItems = true;
                        type("%d. %s (HP: %d, DEF: %d, ATK: %d, Agility: %d, ACC: %d, Equipped: %s)\n",
                             equipmentNum + 1, inv->items[i].name,
                             inv->items[i].data.equipment.hp,
                             inv->items[i].data.equipment.def,
                             inv->items[i].data.equipment.atk,
                             inv->items[i].data.equipment.agility,
                             inv->items[i].data.equipment.acc,
                             inv->items[i].data.equipment.isEquipped ? "Yes" : "No");
                    }
                    equipmentNum++;
                }
                if (!hasItems) {
                    type("No equipment.\n");
                    break;
                }
                type("Do you want to equip an item? (y/n)\n");
                char equipChoice[10];
                if (fgets(equipChoice, sizeof(equipChoice), stdin) == NULL) {
                    type("Error reading input.\n");
                    return;
                }
                equipChoice[strcspn(equipChoice, "\n")] = '\0';
                if (strcmp(equipChoice, "y") == 0 || strcmp(equipChoice, "Y") == 0) {
                    type("Enter the number of the item to equip:\n");
                    char equipInput[10];
                    if (fgets(equipInput, sizeof(equipInput), stdin) == NULL) {
                        type("Error reading input.\n");
                        return;
                    }
                    equipInput[strcspn(equipInput, "\n")] = '\0';
                    int equipIndex = atoi(equipInput) - 1;
                    if (equipIndex < 0 || equipIndex >= equipmentNum) {
                        type("Invalid item number or not equipment.\n");
                    } else {
                        equipmentNum = 0;
                        for (int i = 0; i < inv->itemCount; i++) {
                            if (inv->items[i].itemType == equipment) {
                                equipmentNum++;
                                if (equipmentNum == equipIndex + 1) {
                                    equipItem(p, &inv->items[i]);
                                    type("Equipped %s.\n", inv->items[i].name);
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            case 3:
                int keyItemNum = 0;
                type("Key Items:\n");
                for (int i = 0; i < inv->itemCount; i++) {
                    if (inv->items[i].itemType == keyItem) {
                        hasItems = true;
                        type("%d. %s (Description: %s)\n",
                             keyItemNum + 1, inv->items[i].name,
                             inv->items[i].data.keyItem.description);
                        keyItemNum++;
                    }
                }
                if (!hasItems) type("No key items.\n");
                break;
            case 4:
                int ingredientNum = 0;
                type("Ingredients:\n");
                for (int i = 0; i < inv->itemCount; i++) {
                    if (inv->items[i].itemType == ingredient) {
                        hasItems = true;
                        type("%d. %s (Amount: %d)\n",
                             ingredientNum + 1, inv->items[i].name,
                             inv->items[i].data.ingredient.amount);
                        ingredientNum++;
                    }
                }
                if (!hasItems) type("No ingredients.\n");
                break;
            case 5:
                return;
        }
    }
}

void freeInventory(inventory* inv) {
    if (inv != NULL) {
        for (int i = 0; i < inv->itemCount; i++) {
            free(inv->items[i].name);
            switch (inv->items[i].itemType) {
                case consumable:
                    free(inv->items[i].data.consumable.description);
                    free(inv->items[i].data.consumable.effect);
                    break;
                case equipment:
                    break;
                case keyItem:
                    free(inv->items[i].data.keyItem.description);
                    break;
                case ingredient:
                    break;
            }
        }
        free(inv);
    }
}

void unequipItem(player* p, int equipmentType) {
    if (!p) {
        type("Error: Invalid player\n");
        return;
    }
    if (equipmentType < 0 || equipmentType > 3) {
        type("Error: Invalid equipment type\n");
        return;
    }
    switch (equipmentType) {
        case helmet:
            if (p->equipment.helmet.data.equipment.isEquipped) {
                p->baseStats.hp -= p->equipment.helmet.data.equipment.hp;
                p->baseStats.atk -= p->equipment.helmet.data.equipment.atk;
                p->baseStats.def -= p->equipment.helmet.data.equipment.def;
                p->baseStats.acc -= p->equipment.helmet.data.equipment.acc;
                p->baseStats.agility -= p->equipment.helmet.data.equipment.agility;
                p->stat.hp -= p->equipment.helmet.data.equipment.hp;
                p->stat.atk -= p->equipment.helmet.data.equipment.atk;
                p->stat.def -= p->equipment.helmet.data.equipment.def;
                p->stat.acc -= p->equipment.helmet.data.equipment.acc;
                p->stat.agility -= p->equipment.helmet.data.equipment.agility;
                p->equipment.helmet.data.equipment.isEquipped = false;
                type("Unequipped %s\n", p->equipment.helmet.name);
            } else {
                type("Helmet is not equipped\n");
            }
            break;
        case armour:
            if (p->equipment.armour.data.equipment.isEquipped) {
                p->baseStats.hp -= p->equipment.armour.data.equipment.hp;
                p->baseStats.atk -= p->equipment.armour.data.equipment.atk;
                p->baseStats.def -= p->equipment.armour.data.equipment.def;
                p->baseStats.acc -= p->equipment.armour.data.equipment.acc;
                p->baseStats.agility -= p->equipment.armour.data.equipment.agility;
                p->stat.hp -= p->equipment.armour.data.equipment.hp;
                p->stat.atk -= p->equipment.armour.data.equipment.atk;
                p->stat.def -= p->equipment.armour.data.equipment.def;
                p->stat.acc -= p->equipment.armour.data.equipment.acc;
                p->stat.agility -= p->equipment.armour.data.equipment.agility;
                p->equipment.armour.data.equipment.isEquipped = false;
                type("Unequipped %s\n", p->equipment.armour.name);
            } else {
                type("Armour is not equipped\n");
            }
            break;
        case weapon:
            if (p->equipment.weapon.data.equipment.isEquipped) {
                p->baseStats.hp -= p->equipment.weapon.data.equipment.hp;
                p->baseStats.atk -= p->equipment.weapon.data.equipment.atk;
                p->baseStats.def -= p->equipment.weapon.data.equipment.def;
                p->baseStats.acc -= p->equipment.weapon.data.equipment.acc;
                p->baseStats.agility -= p->equipment.weapon.data.equipment.agility;
                p->stat.hp -= p->equipment.weapon.data.equipment.hp;
                p->stat.atk -= p->equipment.weapon.data.equipment.atk;
                p->stat.def -= p->equipment.weapon.data.equipment.def;
                p->stat.acc -= p->equipment.weapon.data.equipment.acc;
                p->stat.agility -= p->equipment.weapon.data.equipment.agility;
                p->equipment.weapon.data.equipment.isEquipped = false;
                type("Unequipped %s\n", p->equipment.weapon.name);
            } else {
                type("Weapon is not equipped\n");
            }
            break;
        case accessory:
            if (p->equipment.accessory.data.equipment.isEquipped) {
                p->baseStats.hp -= p->equipment.accessory.data.equipment.hp;
                p->baseStats.atk -= p->equipment.accessory.data.equipment.atk;
                p->baseStats.def -= p->equipment.accessory.data.equipment.def;
                p->baseStats.acc -= p->equipment.accessory.data.equipment.acc;
                p->baseStats.agility -= p->equipment.accessory.data.equipment.agility;
                p->stat.hp -= p->equipment.accessory.data.equipment.hp;
                p->stat.atk -= p->equipment.accessory.data.equipment.atk;
                p->stat.def -= p->equipment.accessory.data.equipment.def;
                p->stat.acc -= p->equipment.accessory.data.equipment.acc;
                p->stat.agility -= p->equipment.accessory.data.equipment.agility;
                p->equipment.accessory.data.equipment.isEquipped = false;
                type("Unequipped %s\n", p->equipment.accessory.name);
            } else {
                type("Accessory is not equipped\n");
            }
            break;
    }
}

void equipItem(player* p, item* item) {
    if (!p || !item) {
        type("Error: Invalid player or item\n");
        return;
    }
    if (item->itemType != equipment) {
        type("Error: Item is not equipment\n");
        return;
    }
    if (item->data.equipment.isEquipped) {
        type("Item is already equipped\n");
        return;
    }
    switch (item->data.equipment.type) {
        case helmet:
            if (p->equipment.helmet.data.equipment.isEquipped) {
                unequipItem(p, helmet);
            }
            p->equipment.helmet = *item;
            break;
        case armour:
            if (p->equipment.armour.data.equipment.isEquipped) {
                unequipItem(p, armour);
            }
            p->equipment.armour = *item;
            break;
        case weapon:
            if (p->equipment.weapon.data.equipment.isEquipped) {
                unequipItem(p, weapon);
            }
            p->equipment.weapon = *item;
            break;
        case accessory:
            if (p->equipment.accessory.data.equipment.isEquipped) {
                unequipItem(p, accessory);
            }
            p->equipment.accessory = *item;
            break;
        default:
            type("Error: Unknown equipment type\n");
            return;
    }
    item->data.equipment.isEquipped = true;
    type("Equipped %s\n", item->name);
    p->baseStats.hp += item->data.equipment.hp;
    p->baseStats.atk += item->data.equipment.atk;
    p->baseStats.def += item->data.equipment.def;
    p->baseStats.acc += item->data.equipment.acc;
    p->baseStats.agility += item->data.equipment.agility;
    p->stat.hp += item->data.equipment.hp;
    p->stat.atk += item->data.equipment.atk;
    p->stat.def += item->data.equipment.def;
    p->stat.acc += item->data.equipment.acc;
    p->stat.agility += item->data.equipment.agility;
}

void lvlUp(player* p) {
    type("You leveled up!\nYou are now level %d!\n", p->lvl + 1);
    p->lvl += 1;
    p->xp -= 500;
    p->xen += 4;
    p->baseStats.hp += 50;
    p->baseStats.agility += 3;
    p->baseStats.def += 5;
    p->baseStats.atk += 5;
    p->stat.hp = p->baseStats.hp;
    p->stat.agility = p->baseStats.agility;
    p->stat.def = p->baseStats.def;
    p->stat.atk = p->baseStats.atk;
    type("HP: %d\n", p->baseStats.hp);
    type("Atk: %d\n", p->baseStats.atk);
    type("Def: %d\n", p->baseStats.def);
    type("Agility: %d\n", p->baseStats.agility);
}

void type(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[8096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

#ifndef _WIN32
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif

    for (int i = 0; buffer[i] != '\0'; i++) {
        putchar(buffer[i]);
        fflush(stdout);
#ifdef _WIN32
        if (_kbhit()) {
            int ch = _getch();
            if (ch == '\r') {
                printf("%s", buffer + i + 1);
                break;
            }
        }
#else
        struct pollfd fds[1];
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;
        int ret = poll(fds, 1, 0);
        if (ret > 0 && fds[0].revents & POLLIN) {
            int ch = getchar();
            if (ch == '\n') {
                printf("%s", buffer + i + 1);
                break;
            }
        }
#endif
#ifdef _WIN32
        Sleep(25);
#else
        usleep(25000);
#endif
    }

#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}

void init_combat(player* p, enemy* e) {
    if (!e) return;
    type("A level %d %s appears in front of you.\n", e->lvl, e->name);
    int choice;
    int damage;
    int xp;
    int moveIndex;
    int brnCounter = 0;
    int psnCounter = 0;
    int prsCounter = 0;
    bool isHit;
    while (p->stat.hp > 0 && e->stat.hp > 0) {
        type("Your HP: %d\nXen: %d\n%s's HP: %d\nWhat do you do?\n1. Attack\n2. Run Away\n3. Pray\n4. Chant\n5. Use Item", p->stat.hp, p->xen, e->name, e->stat.hp);
        if (scanf("%d", &choice) != 1) {
            type("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        if (choice == 2) {
            if (p->stat.agility > e->stat.agility) {
                type("Ran away successfully!\n");
                break;
            } else if (p->stat.agility < e->stat.agility) {
                type("%s is too fast. Failed to run away.\n", e->name);
            } else {
                if (rand() % 2 == 0) {
                    type("Ran away successfully!\n");
                    break;
                } else {
                    type("%s is too fast. Failed to run away.\n", e->name);
                }
            }
        } else if (choice == 1) {
            if (p->stat.status.isParalysed && rand() % 2 == 0) type("You are paralyzed you can't move.\n");
            else {
                type("You attacked the %s!\n", e->name);
                damage = p->stat.atk - e->stat.def + (int)ceil(((rand() % 31) / 100.0) * p->stat.atk);
                if (damage < 0) damage = 0;
                e->stat.hp -= damage;
                type("You did %d damage!\n", damage);
            }

        }

        else if (choice == 3) { // Pray
            int prayerOutcome = rand() % 2; // Randomly picks 0 (Blessing) or 1 (Curse)
            
            switch (prayerOutcome) {
                case 0: { // Blessings (50% chance)
                    int grace = rand() % 100;
                    
                    if (grace <= 40) { // 40% chance: Cure all status effects
                        p->stat.status.isBurning = false;
                        p->stat.status.isPoisoned = false;
                        p->stat.status.isParalysed = false;
                        type("The Divine Goddess cures your ailments!\n");
                    } 
                    else if (grace <= 60) { // 20% chance: Smite enemy (25% of their HP)
                        int smiteDmg = e->stat.hp / 2;  //yes much needed actually
                        e->stat.hp -= smiteDmg;
                        type("The Divine Goddess smites your foe for %d damage!\n", smiteDmg);
                    } 
                    else if (grace <= 80) { // 20% chance: Heal player (25% of max HP)
                        int healAmount = p->baseStats.hp / 4;
                        p->stat.hp += healAmount;
                        if (p->stat.hp > p->baseStats.hp) p->stat.hp = p->baseStats.hp;
                        type("The Divine Goddess heals you for %d HP!\n", healAmount);
                    } 
                    else { // 20% chance: Temporary ATK boost (+10)
                        p->stat.atk += 10;
                        type("The Divine Goddess blesses you with holy strength! (+10 ATK)\n");
                    }
                    break;
                }
                
                case 1: { // Curse (50% chance)
                    int jinx = rand() % 100;
                    
                    if (jinx <= 40) { // 40% chance: Burn player
                        p->stat.status.isBurning = true;
                        type("The Divine Goddess' holy flames are purifying you ! (You are now Burning)\n");
                    } 
                    else if (jinx <= 60) { // 20% chance: Lose 25% HP
                        int punishDmg = p->baseStats.hp / 4;
                        p->stat.hp -= punishDmg;
                        type("The Divine Goddess punishes you for %d damage!\n", punishDmg);
                    } 
                    else if (jinx <= 80) { // 20% chance: Paralysis
                        p->stat.status.isParalysed = true;
                        type("The Divine Goddess demands your repentance!\n");
                    } 
                    else { // 20% chance: Both lose 25% HP
                        int mutualDmg = p->baseStats.hp / 4;
                        p->stat.hp -= mutualDmg;
                        e->stat.hp -= mutualDmg;
                        type("The Divine Goddess judges both of you! (-%d HP each)\n", mutualDmg);
                    }
                    break;
                }
            }
        } 

        else if (choice == 4) {   // Chant option
            char input[100];
            
            // Get player input
            type("Enter chant: ");
            getchar(); // Clear input buffer
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0'; // Remove newline

            // Check against all spells
            Spell *matched_spell = NULL;
            for (int i = 0; i < sizeof(allSpells)/sizeof(allSpells[0]); i++) {
                if (strcmp(input, allSpells[i].chant) == 0) {
                    matched_spell = &allSpells[i];
                    break;
                }
            }

            if (!matched_spell) {
                type("Spell Casting Failed!\n");
                p->stat.hp -= 10;
                type("You took 10 damage from magical backlash!\n");
                // DON'T return here - let combat continue
            }
            else if (p->xen < matched_spell->xenreq) {
                type("Spell Casting Failed! (Not enough Xen)\n");
            }
            else {
                // Successful cast
                p->xen -= matched_spell->xenreq;
                int damage = matched_spell->basedamage + (5 * p->lvl);  // +5 DMG PER PLAYER'S LEVEL
                e->stat.hp -= damage;
                
                type("%s was cast successfully! You did %d damage to the enemy.\n",
                    matched_spell->name, damage);
                type("Remaining Xen: %d \n", p->xen);

                // Apply debuffs
                if (matched_spell->debuff.isBurning) {
                    e->stat.status.isBurning = true;
                    type("The enemy bursts into flames!\n");
                }
                if (matched_spell->debuff.isParalysed) {
                    e->stat.status.isParalysed = true;
                    type("The enemy is paralyzed!\n");
                }
            }
            
        }
        else if (choice == 5) { // Use Item
            useItemPanel(p, p->inv);
        }
    
        else {

            type("Invalid choice.\n");
            continue;
        }
        if (e->stat.hp > 0) {
            moveIndex = rand() % 4;
            isHit = (rand() % 100 < e->moveset[moveIndex].acc) ? true : false;
            type("%s used %s\n", e->name, e->moveset[moveIndex].name);
            damage = e->moveset[moveIndex].dmg * (e->stat.atk) / (p->stat.def) + (int)ceil(((rand() % 31) / 100.0) * e->moveset[moveIndex].dmg);
            if (damage < 0) damage = 0;
            if (isHit) {
                if (!strcmp(e->moveset[moveIndex].debuff, "NULL")) { p->stat.hp -= damage; type("You took %d damage!\n", damage); }
                else if (!strcmp(e->moveset[moveIndex].debuff, "brn")) { p->stat.status.isBurning = true; brnCounter = 0; p->stat.hp -= damage; type("You took %d damage!\n", damage); }
                else if (!strcmp(e->moveset[moveIndex].debuff, "psn")) { p->stat.status.isPoisoned = true; psnCounter = 0; p->stat.hp -= damage; type("You took %d damage!\n", damage); }
                else if (!strcmp(e->moveset[moveIndex].debuff, "prs")) { p->stat.status.isParalysed = true; prsCounter = 0; p->stat.hp -= damage; type("You took %d damage!\n", damage); }
                else if (!strcmp(e->moveset[moveIndex].debuff, "atkBuff")) { e->stat.atk += 10; type("%s's attack went up!\n", e->name); }
            } else {
                type("%s missed!\n", e->name);
            }
        }
        if (p->stat.status.isBurning && brnCounter < 3) {
            type("You are burning!\n");
            damage = (int)ceil(p->baseStats.hp / 16);
            p->stat.hp -= damage;
            type("You took %d damage!\n", damage);
            brnCounter += 1;
        }
        if (p->stat.status.isPoisoned && psnCounter < 3) {
            type("You are poisoned!\n");
            damage = (int)ceil(p->baseStats.hp / 16);
            p->stat.hp -= damage;
            type("You took %d damage!\n", damage);
            psnCounter += 1;
        }
        if (p->stat.status.isParalysed && prsCounter < 3) {
            type("You are paralysed!\n");
            psnCounter += 1;
        }
        if (brnCounter == 3) p->stat.status.isBurning = false; brnCounter = 0;
        if (psnCounter == 3) p->stat.status.isPoisoned = false; psnCounter = 0;
        if (prsCounter == 3) p->stat.status.isParalysed = false; prsCounter = 0;
    }
    if (p->stat.hp <= 0) {
        type("You died!\n");
        p->gameTriggers[isAlive] = false;
    } 
    else if (e->stat.hp <= 0) {
        type("You killed the %s!\n", e->name);
        xp = (e->xpSeed + e->xpSeed * sqrt(e->lvl / p->lvl)) * 2 * (1 - (p->lvl / MAX_LEVEL));
        int i = 0;
        while(i < e->drops.itemCount) {
            if(rand() % 100 < e->dropRate[i]) {
                addItem(p->inv, e->drops.items[i]);
                type("It dropped %s!\n", e->drops.items[i].name);
            } else {
                type("It did not drop anything\n");
            }
            i++;
        }
        p->xp += xp;
        type("You gained %d XP!\n", xp);
        p->stat.status.isPoisoned = false;
        p->stat.status.isParalysed = false;
        p->stat.status.isBurning = false;
        p->stat.status.isFrozen = false;
        if (p->xp >= 500) lvlUp(p);
    }
    freeEnemy(e);
}

enemy* createEnemy(player* p, int enemyIndex) {
    if (enemyIndex == randomEnemy) enemyIndex = rand() % 2;
    enemy* newEnemy = malloc(sizeof(enemy));
    if (!newEnemy) return NULL;

    //Enemy Stats

    newEnemy->name = strdup(allEnemy[enemyIndex].name);
    newEnemy->type = strdup(allEnemy[enemyIndex].type);
    newEnemy->lvl = p->lvl + (rand() % 5) - 2;
    if (newEnemy->lvl < 1) newEnemy->lvl = 1;
    newEnemy->stat.hp = allEnemy[enemyIndex].stat.hp + newEnemy->lvl * 15;
    newEnemy->stat.def = allEnemy[enemyIndex].stat.def + newEnemy->lvl * 2;
    newEnemy->stat.atk = allEnemy[enemyIndex].stat.atk + newEnemy->lvl * 3;
    newEnemy->stat.agility = allEnemy[enemyIndex].stat.agility + newEnemy->lvl * 3;
    newEnemy->stat.acc = allEnemy[enemyIndex].stat.acc;
    newEnemy->stat.status = (status){ false, false, false, false };
    newEnemy->xpSeed = allEnemy[enemyIndex].xpSeed;

    //Enemy Moveset

    for (int i = 0; i < 4; i++) {
        if (allEnemy[enemyIndex].moveset[i].name) {
            newEnemy->moveset[i].name = strdup(allEnemy[enemyIndex].moveset[i].name);
            newEnemy->moveset[i].dmg = allEnemy[enemyIndex].moveset[i].dmg;
            newEnemy->moveset[i].acc = allEnemy[enemyIndex].moveset[i].acc;
            newEnemy->moveset[i].lvlReq = allEnemy[enemyIndex].moveset[i].lvlReq;
            newEnemy->moveset[i].debuff = strdup(allEnemy[enemyIndex].moveset[i].debuff);
        }
    }

    //Enemy Drops

    inventory* tempInv = createInventory();
    if (!tempInv) {
        free(newEnemy->name);
        free(newEnemy->type);
        for (int i = 0; i < 4; i++) {
            if (newEnemy->moveset[i].name) {
                free(newEnemy->moveset[i].name);
                free(newEnemy->moveset[i].debuff);
            }
        }
        free(newEnemy);
        return NULL;
    }
    newEnemy->drops = *tempInv;
    free(tempInv);
    if (enemyIndex == slime) {
        addItem(&newEnemy->drops, consumables[0]);
        addItem(&newEnemy->drops, consumables[1]);
        newEnemy->dropRate[0] = 40;
        newEnemy->dropRate[1] = 20;
    }
    else if (enemyIndex == goblin) {
        addItem(&newEnemy->drops, consumables[0]);
        addItem(&newEnemy->drops, consumables[1]);
        newEnemy->dropRate[0] = 50;
        newEnemy->dropRate[1] = 30;
    } else if (enemyIndex == cyclops) {
        addItem(&newEnemy->drops, equipments[1]);
        addItem(&newEnemy->drops, consumables[0]);
        addItem(&newEnemy->drops, consumables[1]);
        newEnemy->dropRate[0] = 100;
        newEnemy->dropRate[1] = 80;
        newEnemy->dropRate[2] = 60;
    }
    return newEnemy;
}

player* createPlayer() {
    player* p = malloc(sizeof(player));
    if (!p) return NULL;
    p->inv = createInventory();
    if (!p->inv) {
        free(p);
        return NULL;
    }
    for (int i = 0; i < NUM_TRIGGERS; i++) {
        p->gameTriggers[i] = (i == isAlive) ? true : false;
    }
    p->lvl = 1;
    p->xp = 0;
    p->xen = 30;
    p->baseXen = 30;
    p->baseStats = (stats){200, 20, 30, 100, 20, (status){false, false, false, false}};
    p->stat = (stats){200, 20, 30, 100, 20, (status){false, false, false, false}};
    p->equipment.helmet = (item){ .itemType=equipment, .data.equipment = { .isEquipped = false } };
    p->equipment.armour = (item){ .itemType=equipment, .data.equipment = { .isEquipped = false } };
    p->equipment.weapon = (item){ .itemType=equipment, .data.equipment = { .isEquipped = false } };
    p->equipment.accessory = (item){ .itemType=equipment, .data.equipment = { .isEquipped = false } };

    return p;
}

scene* createScene(int numChoices, char* description, char** choices, bool* choiceConditions, int* choiceIds, scene** nextScenePerChoice, int sceneNo) {
    scene* newScene = malloc(sizeof(scene) + sizeof(char*) * numChoices);
    if (!newScene) return NULL;
    newScene->choiceConditions = malloc(sizeof(bool) * numChoices);
    if (!newScene->choiceConditions) {
        free(newScene);
        return NULL;
    }
    newScene->choiceIds = malloc(sizeof(int) * numChoices);
    if (!newScene->choiceIds) {
        free(newScene->choiceConditions);
        free(newScene);
        return NULL;
    }
    newScene->nextScenePerChoice = malloc(sizeof(scene*) * numChoices);
    if (!newScene->nextScenePerChoice) {
        free(newScene->choiceIds);
        free(newScene->choiceConditions);
        free(newScene);
        return NULL;
    }
    newScene->description = description ? strdup(description) : NULL;
    newScene->numChoices = numChoices;
    newScene->sceneNo = sceneNo;
    for (int i = 0; i < numChoices; i++) {
        newScene->choices[i] = choices && choices[i] ? strdup(choices[i]) : NULL;
        newScene->choiceConditions[i] = choiceConditions ? choiceConditions[i] : false;
        newScene->choiceIds[i] = choiceIds ? choiceIds[i] : (sceneNo * 100 + (i + 1));
        newScene->nextScenePerChoice[i] = nextScenePerChoice ? nextScenePerChoice[i] : NULL;
    }
    return newScene;
}

scene* processChoice(scene* currentScene, player* p, int choiceIndex) {
    if (!currentScene || !p || choiceIndex < 0 || choiceIndex >= currentScene->numChoices) {
        type("Invalid choice index!\n");
        return currentScene;
    }
    if (!currentScene->choiceConditions[choiceIndex]) {
        type("Choice not available!\n");
        return currentScene;
    }
    int choiceId = currentScene->choiceIds[choiceIndex];
    int choiceNumber = choiceId % 100;
    switch (choiceId) {
        case 101:
            if (!p->gameTriggers[hasAxe]) {
                addItem(p->inv, equipments[0]);
                p->gameTriggers[hasAxe] = true;
            }
            return currentScene->nextScenePerChoice[choiceNumber - 1];
        case 102:
            if (!p->gameTriggers[hasKey1]) {
                addItem(p->inv, keyItems[0]);
                p->gameTriggers[hasKey1] = true;
            }
            return currentScene->nextScenePerChoice[choiceNumber - 1];
        case 103:
            if (p->gameTriggers[hasKey1]) {
                type("Gate unlocked with Key1!\n");
                return currentScene->nextScenePerChoice[choiceNumber - 1];
            } else {
                type("Need Key1 to unlock gate.\n");
                return currentScene;
            }
        case 104:
            type("You open the chest and find some xen potions.\n");
            addItem(p->inv, consumables[1]);
            currentScene->choiceConditions[3] = false; // Disable this choice after using it
            return currentScene->nextScenePerChoice[choiceNumber - 1];
        case 201:
            if (p->gameTriggers[hasAxe]) {
                type("Door chopped with Axe!\n");
                return currentScene->nextScenePerChoice[choiceNumber - 1];
            } else {
                type("Need Axe to chop door.\n");
                return currentScene;
            }
        case 202:
            type("You go back to the dark room.\n");
            return currentScene->nextScenePerChoice[choiceNumber - 1];
        case 203:
            init_combat(p, createEnemy(p, randomEnemy));
            if (!p->gameTriggers[isAlive]) {
                type("Game Over.\n");
                return NULL;
            }
            return currentScene;
        case 301:
            init_combat(p, createEnemy(p, cyclops));
            if (!p->gameTriggers[isAlive]) {
                type("Game Over.\n");
                return NULL;
            }
            p->gameTriggers[cyclopsKilled] = true;
            return currentScene;
        case 302:
        case 303:
            return currentScene->nextScenePerChoice[choiceNumber - 1];
        default:
            type("Invalid choice ID.\n");
            return currentScene;
    }
}

void displayScene(scene* currentScene, player* p) {
    if (!currentScene || !p) return;
    char temp[8096] = { 0 };
    int len;
    snprintf(temp, sizeof(temp), "\n%s\nWhat do you do?\n", currentScene->description);
    len = strlen(temp);
    for (int i = 0; i < currentScene->numChoices; i++) {
        if (currentScene->choiceConditions[i]) {
            char choice[512];
            snprintf(choice, sizeof(choice), "%d. %s\n", i + 1, currentScene->choices[i]);
            strncat(temp, choice, sizeof(temp) - len - 1);
            len += strlen(choice);
        }
    }
    type("%s", temp);
}

void updateSceneConditions(scene* currentScene, player* p) {
    if (!currentScene || !p) return;
    for (int i = 0; i < currentScene->numChoices; i++) {
        switch (currentScene->choiceIds[i]) {
            case 101:
                currentScene->choiceConditions[i] = !p->gameTriggers[hasAxe];
                break;
            case 102:
                currentScene->choiceConditions[i] = !p->gameTriggers[hasKey1];
                break;
            case 103:
                currentScene->choiceConditions[i] = true;
                break;
            case 201:
                currentScene->choiceConditions[i] = true;
                break;
            case 202:
                currentScene->choiceConditions[i] = true;
                break;
            case 203:
                currentScene->choiceConditions[i] = true;
                break;
            case 302:
                currentScene->choiceConditions[i] = p->gameTriggers[cyclopsKilled];
                break;
            case 303:
                currentScene->choiceConditions[i] = p->gameTriggers[cyclopsKilled];
                break;
            default:
                break;
        }
    }
}

void freeScene(scene* s) {
    if (s) {
        for (int i = 0; i < s->numChoices; i++) {
            if (s->choices[i]) free(s->choices[i]);
        }
        if (s->description) free(s->description);
        if (s->choiceConditions) free(s->choiceConditions);
        if (s->choiceIds) free(s->choiceIds);
        if (s->nextScenePerChoice) free(s->nextScenePerChoice);
        free(s);
    }
}

void freePlayer(player* p) {
    if (!p) return;
    if (p->inv) {
        freeInventory(p->inv);
        p->inv = NULL;
    }
    free(p);
}

void freeEnemy(enemy* e) {
    if (e) {
        free(e->name);
        free(e->type);
        for (int i = 0; i < 4; i++) {
            free(e->moveset[i].name);
            free(e->moveset[i].debuff);
        }
        if(&e->drops != NULL) {
            for (int i = 0; i < e->drops.itemCount; i++) {
                free(e->drops.items[i].name);
                switch (e->drops.items[i].itemType) {
                    case consumable:
                        free(e->drops.items[i].data.consumable.description);
                        free(e->drops.items[i].data.consumable.effect);
                        break;
                    case keyItem:
                        free(e->drops.items[i].data.keyItem.description);
                        break;
                    default:
                        break;
                }
            }
        }
        free(e);
    }
}

int main() {
    srand(time(NULL));
    player* p = createPlayer();
    if (!p) return 1;

    const int numScenes = 3;
    scene* scenes[numScenes];
    scene** nextScenes[numScenes];

    char* choices1[] = { "Pick up axe", "Pick up key1", "Unlock gate", "Open Chest", NULL };
    bool conditions1[] = { true, true, true, true };
    scene* nextScenes1[4] = { NULL, NULL, NULL, NULL };

    char* choices2[] = { "Chop door", "Go back to dark room", "Fight the enemy", NULL };
    bool conditions2[] = { true, true, true };
    scene* nextScenes2[3] = { NULL, NULL, NULL };

    char* choices3[] = { "Fight the cyclops", "Go back to previous room", "End Game", NULL };
    bool conditions3[] = { true, true, true };
    scene* nextScenes3[3] = { NULL, NULL, NULL };

    scenes[0] = createScene(4, "A dark room with an axe, a chest and a gate.", choices1, conditions1, NULL, nextScenes1, 1);
    scenes[1] = createScene(3, "A room with a wooden door and an enemy lurking.", choices2, conditions2, NULL, nextScenes2, 2);
    scenes[2] = createScene(3, "There is an angry Cyclops in the room", choices3, conditions3, NULL, nextScenes3, 3);
    for (int i = 0; i < numScenes; i++) {
        if (!scenes[i]) {
            for (int j = 0; j < i; j++) freeScene(scenes[j]);
            freePlayer(p);
            return 1;
        }
    }

    nextScenes[0] = nextScenes1;
    nextScenes[1] = nextScenes2;
    nextScenes[2] = nextScenes3;

    nextScenes1[0] = scenes[0];
    nextScenes1[1] = scenes[0];
    nextScenes1[2] = scenes[1];
    nextScenes1[3] = scenes[0];

    nextScenes2[0] = scenes[2];
    nextScenes2[1] = scenes[0];
    nextScenes2[2] = scenes[1];

    nextScenes3[0] = scenes[2];
    nextScenes3[1] = scenes[1];
    nextScenes3[2] = NULL;

    for (int i = 0; i < numScenes; i++) {
        for (int j = 0; j < scenes[i]->numChoices; j++) {
            scenes[i]->nextScenePerChoice[j] = nextScenes[i][j];
        }
    }

    scene* currentScene = scenes[0];
    int choice;
    char input[10];

    while (currentScene && p->gameTriggers[isAlive]) {
        updateSceneConditions(currentScene, p);
        displayScene(currentScene, p);
        type("Enter choice (1-%d, 0 to quit, 'i' for inventory, 'p' for stats): ", currentScene->numChoices);
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            bool isEmpty = true;
            for (int i = 0; input[i] != '\0'; i++) {
                if (!isspace(input[i])) {
                    isEmpty = false;
                    break;
                }
            }
            if (isEmpty) {
                continue;
            }
            if (strcmp(input, "i") == 0) {
                displayInventory(p->inv, p);
                continue;
            } else if (strcmp(input, "p") == 0) {
                type("Your Level: %d\n", p->lvl);
                type("Your XP: %d\n", p->xp);
                type("Your HP: %d\n", p->stat.hp);
                type("Your Atk: %d\n", p->stat.atk);
                type("Your Def: %d\n", p->stat.def);
                type("Your Agility: %d\n", p->stat.agility);
                continue;
            } else {
                choice = atoi(input);
                if (choice < 0 || choice > currentScene->numChoices) {
                    type("Invalid input! Please enter a number between 0 and %d.\n", currentScene->numChoices);
                    continue;
                }
                if (choice == 0) break;
                currentScene = processChoice(currentScene, p, choice - 1);
            }
        } else {
            type("Error reading input!\n");
            continue;
        }
    }
    if (!p->gameTriggers[isAlive]) {
        type("Game Over.\n");
    }

    for (int i = 0; i < numScenes; i++) {
        freeScene(scenes[i]);
    }
    freePlayer(p);
    return 0;
}
