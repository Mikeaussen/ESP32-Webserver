// Beispiel:
// const config = await UserConfig.load("./user/users.json");
// config.updateParam(2, "pin", "9999");
// const updated = config.save();

class UserConfig
{
  constructor(data, sourceUrl = null)
  {
    this.data = data;
    this.sourceUrl = sourceUrl; // merkt sich, woher die JSON kam
    this.data = data;
  }

  static async load(url)
  {
    const res = await fetch(url);
    if (!res.ok) throw new Error(`Konnte ${url} nicht laden`);
    const data = await res.json();
    return new UserConfig(data, url);
  }

  findUser(nr)
  {
    return this.data.user.find(u => u.nr === nr) || null;
  }

  updateParam(nr, key, value)
  {
    const user = this.findUser(nr);
    if (!user) throw new Error(`Kein Benutzer mit nr ${nr} gefunden`);
    user[key] = value;
  }

  addUser(userObj)
  {
    if (!userObj.nr) throw new Error("Benutzer braucht eine 'nr'");
    if (this.findUser(userObj.nr)) throw new Error("Benutzernummer existiert bereits");
    this.data.user.push(userObj);
  }

  deleteUser(nr)
  {
    const nrx = this.data.user.findIndex(u => u.nr === nr);
    if (nrx === -1) throw new Error(`Kein Benutzer mit nr ${nr} gefunden`);
    this.data.user.splice(nrx, 1);
  }

  save()
  {
    // gibt nur den JSON-String zurück. Speichern auf dem Server geht nur über Backend.
    return JSON.stringify(this.data, null, 2);
  }
}