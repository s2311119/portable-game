#pragma once

#include "Item.h"

class Inventory;

struct ShopPurchaseQuote {
  Item item;
  bool affordable;
};

class ShopTransactionSystem {
public:
  static ShopPurchaseQuote quotePurchase(int itemId,
                                        const Inventory& inventory);
  static void completePurchase(const ShopPurchaseQuote& quote,
                               Inventory& inventory);
  static int salePriceAt(const Inventory& inventory, int inventoryIndex);
  static void completeSale(Inventory& inventory, int inventoryIndex,
                           int salePrice);
};
