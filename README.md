# OTA GPRS

Modifikasi dari [TTGO - LilyGo-T-Call-SIM800](https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/tree/master/examples/Arduino_GSM_OTA)
## Attention

**yang perlu diperhatikan bandwith nya**

Bakal sering timeout kemungkinan karena time out untuk fetching raw datanya kurang dibesarin, dalam contoh repo ini disetting 100000L.

Bisa dimodif dibagian pembanding timeout berikut :

```cpp
if (millis() - timeout > 100000L) {
    DEBUG_FATAL("Timeout");
}
```

## TODO

0. [x] Implementasi 
0. [ ] OTA Service
0. [ ] Create class
0. [ ] Deploy to arduino.info

## Contribute

If you like - give it a star, or fork it and contribute!

## Give me Coffe 

[Saweria https://saweria.co/mamunsyuhada](https://saweria.co/mamunsyuhada)

[![Saweria](docs/qrsaweria.png)](https://saweria.co/mamunsyuhada)

