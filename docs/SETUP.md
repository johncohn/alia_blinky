# "Your own sparkly Alia" Sign-Up Page: Setup

`docs/index.html` is a one-page site for the ornament, served by GitHub Pages at
**https://johncohn.github.io/alia_blinky/**. Buyers fill in their details, choose how many (and how many with a diffuser), and tap one button: **Order & pay now with Venmo** (opens Venmo with the recipient and amount filled in) or **Order & pay at pickup**. On phones, a floating "Order yours ↓" button points to the form until it's on screen.

Other files:

| File | What it is |
|------|------------|
| `flyer.html` | One-page printable flyer (https://johncohn.github.io/alia_blinky/flyer.html). Open it and press ⌘P. |
| `sparkly-alia-flyer.pdf` | The same flyer as a ready-to-print PDF (US Letter) |
| `qr-signup.png` | QR code to the sign-up page, with a full white margin so phones scan it easily (used on the flyer) |
| `flyer-photo.jpg` | Still from the video, used on the flyer |
| `fonts/fredoka-latin.woff2` | The Fredoka font, hosted here (see Notes) |

To remake the PDF after editing the flyer:

```
"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" --headless --no-pdf-header-footer \
  --print-to-pdf=docs/sparkly-alia-flyer.pdf "file://$PWD/docs/flyer.html"
```

## Where sign-ups go

Every sign-up becomes a row in a **Google Sheet that you own**, through a small Google Apps Script ([`apps-script.gs`](apps-script.gs)). The script also sends emails:

| When | Who gets it | What's in it |
|------|-------------|--------------|
| Someone signs up | **The buyer** | Their order (including how many with a diffuser) and total, pickup info, your contact details, and the Toys for Tots match. Venmo payers get a thank-you; pay-at-pickup buyers get a **Pay with Venmo** button and QR code in case they'd like to pay early |
| Someone signs up | **You** | Their name, email, phone, quantity, diffusers, how they're paying, and message |
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

- New sign-ups appear as rows on the **Signups** tab. **Diffusers** says how many of that order should have a diffuser, and **Pays by** says "Venmo (now)" or "At pickup". (These columns are at the end so older sheets keep working; the script adds the headers itself.)
- "Venmo (now)" means they were sent to Venmo, not that the money arrived. Check Venmo before ticking **Paid?**.
- When someone's Venmo payment arrives, tick **Paid?** on their row. They get a payment confirmation, and the time it was sent appears in **Paid email sent**. It's only ever sent once per row.
- If someone pays at pickup, tick **Paid?** then too, if you'd like them to get the email.
- Gmail lets a script send about **100 emails a day** on a personal account. That's plenty here, but worth knowing.

## 3. Venmo

`VENMO_USER` is set in both `index.html` and the script. `docs/venmo-qr.png` is your Venmo QR code; the confirmation emails load it from the live site, so it only shows up in emails once GitHub Pages is on.

## 4. The video

The page plays **`docs/alia.mp4`**, a short silent looping video of two ornaments (with and without diffusers), with **`docs/alia-poster.jpg`** shown while it loads. The "With diffuser" and "Without" labels sit on top of it. To swap in a new phone video (`.mov`), shrink it with the commands below. `transpose=2` rotates it a quarter turn; drop it if the video is already the right way up.

```
ffmpeg -i NEW.mov -an -vf "transpose=2,scale=960:-2,format=yuv420p" -c:v libx264 -crf 27 -movflags +faststart docs/alia.mp4
ffmpeg -ss 1 -i docs/alia.mp4 -frames:v 1 docs/alia-poster.jpg
ffmpeg -ss 1 -i NEW.mov -frames:v 1 -vf "transpose=2,scale=1600:-2,eq=saturation=1.35:contrast=1.06" docs/flyer-photo.jpg
```

If the new video isn't 960×584, change the `aspect-ratio` on `.hero video` in `index.html` to match. Keep it under about 3 MB so it loads quickly on phones.

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
- **No outside fonts or scripts.** Safari's Advanced Privacy Protection (and some ad blockers) block Google Fonts, which left iPhones on a blank page with a "Reduce Protections" warning. The font is now served from `docs/fonts/`, so keep it that way.
- **Changes take up to 10 minutes to show.** GitHub Pages lets browsers cache pages for 10 minutes. Press ⌘⇧R, or add `?v=2` to the address, to see the latest.
- **The page always shows "Yay, you're in!" once a submission is sent.** Apps Script doesn't let the page read its reply, so if the sheet ever stops receiving rows, the page can't tell. Check the sheet after setting it up.
