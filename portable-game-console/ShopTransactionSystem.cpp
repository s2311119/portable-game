#include "ShopTransactionSystem.h"

#include "Inventory.h"
#include "ItemCatalog.h"

ShopPurchaseQuote ShopTransactionSystem::quotePurchase(
    int itemId, const Inventory& inventory) {
  Item item = ItemCatalog::get(itemId);
  return {item, inventory.money >= item.price};
}

void ShopTransactionSystem::completePurchase(const ShopPurchaseQuote& quote,
                                             Inventory& inventory) {
  inventory.money -= quote.item.price;
  inventory.addItem(quote.item);
}

int ShopTransactionSystem::salePriceAt(const Inventory& inventory,
                                       int inventoryIndex) {
  int salePrice = inventory.items[inventoryIndex].item.price / 2;
  if (salePrice < 1) salePrice = 1;
  return salePrice;
}

void ShopTransactionSystem::completeSale(Inventory& inventory,
                                         int inventoryIndex, int salePrice) {
  inventory.money += salePrice;
  inventory.removeByIndex(inventoryIndex);
}
