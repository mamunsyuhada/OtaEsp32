# Riset OTA

## Attention

**yang perlu diperhatikan bandwith nya**

Bakal sering timeout kemungkinan karena time out untuk fetching raw datanya kurang dibesarin, dalam contoh repo ini disetting 100000L.

Bisa dimodif dibagian pembanding timeout berikut :

```cpp
if (millis() - timeout > 100000L) {
    DEBUG_FATAL("Timeout");
}
```

# Contribute

If you like TinyGSM - give it a star, or fork it and contribute!

# Give me Coffe 

[![Saweria](https://saweria.co/overlays/qr?streamKey=45635790ec9aa2d04fb5c4abb0eedfd6&backgroundColor=%23faae2b&barcodeColor=%23000&username=mamunsyuhada)](https://saweria.co/mamunsyuhada)

