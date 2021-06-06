#ifndef PADDER_HH
#define PADDER_HH

#include "IEncoder.hh"

#include <memory>
#include <vector>

class Padder : public IEncoder
{
   static const uint16_t mEncoderId = 0x0003;

 public:
   enum PaddingType
   {
      None = 0,
      WholeBytes = 1,
      EvenBytes = 2,
      OddBytes = 3
   };

   Padder(PaddingType);
   ~Padder(){};

   uint16_t getEncoderId() const override { return mEncoderId; };
   static Padder* deserializerFactory(const bitSet&);

   // Inherited functions from IEncoder
   bitSet encode(const bitSet&) override;
   bitSet decode(const bitSet&) override;
   bitSet serialize() const override;
   size_t getTableSize() const override;
   bool isValid() const override;
   std::map<bitSet, bitSet> getEncodingMap() const override { return std::map<bitSet, bitSet>(); };
   void setup(const bitSet&) override;
   void reset() override;

 private:
   PaddingType mPaddingMode;
   uint32_t mAddedBits;
};

#endif // PADDER_HH