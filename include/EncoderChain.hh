#ifndef ENCODERCHAIN_HH
#define ENCODERCHAIN_HH

#include "IEncoder.hh"

#include <memory>
#include <vector>

class EncoderChain : public IEncoder
{
 public:
   EncoderChain();
   ~EncoderChain(){};

   void addEncoder(std::unique_ptr<IEncoder>&&);
   uint16_t getEncoderId() const override { return 0x0000; };
   static uint16_t readEncoderId(const bitSet&);
   static EncoderChain* deserializerFactory(const bitSet&);

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
   std::vector<std::unique_ptr<IEncoder>> mEncoderChain;
};

#endif // ENCODERCHAIN_HH