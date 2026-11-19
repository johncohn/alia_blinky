# "Your own sparkly Alia" Sign-Up Page: Setup

`docs/index.html` is a one-page site for the ornament, served by GitHub Pages at
**https://johncohn.github.io/alia_blinky/**. `docs/qr_ornament_page.png` is a QR code for that address. **`docs/flyer.html`** (https://johncohn.github.io/alia_blinky/flyer.html) is a one-page printable flyer with a QR code to the sign-up page: open it and press ⌘P.

## Where sign-ups go

Every sign-up becomes a row in a **Google Sheet that you own**, through a small Google Apps Script ([`apps-script.gs`](apps-script.gs)). The script also sends emails:

| When | Who gets it | What's in it |
|------|-------------|--------------|
| Someone signs up | **The buyer** | Their order and total, pickup info, your contact details, a **Pay with Venmo** button with the amount filled in, your Venmo QR code, and "pay now or when your Alia is delivered" |
| Someone signs up | **You** | Their name, email, phone, quantity and message |
| You tick **Paid?** on their row | **The buyer** | "Payment received, thank you!", plus how much that means for Toys for Tots |

The page itself stores nothing.

## 1. Set up the Google Sheet (about 5 minutes)

1. Create a new Google Sheet, for example "Sparkly Alia Sign-ups".
2. Open **Extensions → Apps Script**.
3. Delete what's there and paste in the script. The repo copy has placeholder contact details; either fill in `CONTACT_EMAIL` and `CONTACT_PHONE` at the top, or paste a copy you've already filled in. Click **Save**.
4. Click **Deploy → New deployment**. Click the gear icon, choose **Web app**, then set:
   - **Execute as:** Me
   - **Who has access:** Anyone
5. Click **Deploy** and approve the permissions. These cover the sheet, sending email, and triggers. Google may say the app is "unverified" because it's your own script; choose **Advanced → Go to … (unsafe)** to continue.
6. Copy the **Web app URL** (it ends in `/exec`) and paste it into `SHEET_URL` near the bottom of `index.html`:
   ```js
   SHEET_URL: "https://script.google.com/macros/s/…/exec",
   ```
7. **Turn on payment emails:** reload the Google Sheet. A **Sparkly Alia** menu appears; choose **Sparkly Alia → Turn on payment emails** and approve. You only need to do this once.

If you change the script later, use **Deploy → Manage deployments → Edit → New version** so the URL stays the same.

## 2. Day to day

- New sign-ups appear as rows on the **Signups** tab. The **Diffusers** column (last) says how many of that order should have a diffuser.
- When someone's Venmo payment arrives, tick **Paid?** on their row. They get a payment confirmation, and the time it was sent appears in **Paid email sent**. It's only ever sent once per row.
- If someone pays at pickup, tick **Paid?** then too, if you'd like them to get the email.
- Gmail lets a script send about **100 emails a day** on a personal account. That's plenty here, but worth knowing.

## 3. Venmo

`VENMO_USER` is set in both `index.html` and the script. `docs/venmo-qr.png` is your Venmo QR code; the confirmation emails load it from the live site, so it only shows up in emails once GitHub Pages is on.

## 4. The video

The page plays **`docs/alia.mp4`**, a short silent looping video, with **`docs/alia-poster.jpg`** shown while it loads. To swap in a new phone video (`.mov`), shrink it with:

```
ffmpeg -i NEW.mov -an -vf "scale=720:-2,format=yuv420p" -c:v libx264 -crf 27 -movflags +faststart docs/alia.mp4
ffmpeg -ss 1 -i docs/alia.mp4 -frames:v 1 docs/alia-poster.jpg
```

If the new video isn't 1138×848, change the `aspect-ratio` on `.hero video` in `index.html` to match. Keep it under about 3 MB so it loads quickly on phones.

## 5. Turn on GitHub Pages

In the repo on GitHub, go to **Settings → Pages → Build and deployment**:
- **Source:** Deploy from a branch
- **Branch:** `main`, folder **`/docs`**

The site goes live at https://johncohn.github.io/alia_blinky/ within a minute or two.

## 6. Test it

Sign up on the live page using your own email:
1. A row should appear in the sheet.
2. You should get two emails: the buyer confirmation and the owner notification.
3. Tick **Paid?** on that row; you should get the payment email.

Delete the test row afterwards.

## Notes

- **Privacy:** buyers' details go only to your Google Sheet.
- **Spam:** a hidden field catches simple bots.
- **The page always shows "Yay, you're in!" once a submission is sent.** Apps Script doesn't let the page read its reply, so if the sheet ever stops receiving rows, the page can't tell. Check the sheet after setting it up.
