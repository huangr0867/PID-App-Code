import { Component, NgZone } from '@angular/core';
import { BLE } from '@ionic-native/ble/ngx';
import { Router, NavigationExtras } from '@angular/router';

@Component({
  selector: 'app-home',
  templateUrl: 'home.page.html',
  styleUrls: ['home.page.scss'],
})
export class HomePage {

  devices: any[] = [];
  
  constructor(private ble: BLE, private ngZone: NgZone, private router: Router) {}

  ionViewDidEnter() {
    console.log('ionViewDidEnter');
    this.scan();
  }

  scan() {
    this.devices = [];
    this.ble.scan([], 5).subscribe(
      device => this.onDeviceDiscovered(device)
    );
  }

  onDeviceDiscovered(device) {
    console.log('Discovered' + JSON.stringify(device, null, 2));
    this.ngZone.run(()=>{
      this.devices.push(device)
      console.log(device)
    })
  }

  deviceSelected(device) {
    //alert('clicked: '+ device.id);
    this.openDetail(device);
  }

  openDetail(device) {
    let navigationExtras: NavigationExtras = {
      state: {
        device: device
      }
    };
    this.router.navigate(['detail'], navigationExtras);
  }
}